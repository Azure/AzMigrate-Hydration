//
// fromheader.cpp: Create serialize*.{cpp,h} files from config header files
//
// Limitations:
// - Does not use pre-processor to do conditional compilation
// - Does not handle nested structure definitions.
// - Does not handle forward template declarations
//
#include <map>
#include <vector>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <cstring>
#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_functions.hpp>
#include "inmsafecapis.h"


using namespace BOOST_SPIRIT_CLASSIC_NS;
using namespace phoenix;
using namespace std;

struct HeaderContext {
    string filename;
    map< string, vector<string> > info;
    vector<string> includes;
};
static bool enableDebug = false;
struct SlurpException : std::exception
{
	virtual const char* what() const throw()
	{
		return "couldn't slurp file contents";
	}
};

struct ParseException : std::exception
{
	virtual const char* what() const throw()
	{
		return "couldn't parse header file";
	}
};

struct store_field_impl
{
    template <typename Container, typename Key, typename Item>
    struct result
    {
        typedef void type;
    };

    template <typename Container, typename Key, typename Item>
    void operator()(Container& c, Key const& key, Item item) const
    {
        c[ key ].push_back( item );
    }
};
phoenix::function<store_field_impl> const store_field_a = store_field_impl();

typedef void (*Printer_t)(HeaderContext const&);
typedef std::map<std::string, Printer_t> OutputDispatcher_t;

string slurpFile( const char* filename ) {
    FILE* ifp = fopen( filename, "r" );
    if( NULL == ifp ) {
        throw SlurpException();
    }
    size_t count;
    char buffer[ 8192 ];
    string result;

    while( (count = fread( buffer, 1, sizeof( buffer ) - 1 , ifp )) > 0 ) {
        buffer[ count ] = 0;
        result += buffer;
    }

    fclose( ifp );
    return result;
}

struct CppGrammar : grammar<CppGrammar> {
    template <typename ScannerT>
    struct definition {
        definition( CppGrammar const& self ) {
            typedefDefinition = 
                str_p( "typedef" ) >> lexeme_d[ +(~ch_p(';')) ] >> ';';

            operatorDefinition = 
                str_p("operator") >> 
                    ((str_p("new")>>!str_p("[]"))
                    |(str_p("delete")>>!str_p("[]"))
                    |str_p("->*")
                    |str_p("<<=")
                    |str_p(">>=")
                    |str_p("+=")
                    |str_p("-=")
                    |str_p("<<")
                    |str_p(">>")
                    |str_p("==")
                    |str_p("!=")
                    |str_p("<=")
                    |str_p(">=")
                    |str_p("++")
                    |str_p("--")
                    |str_p("&=")
                    |str_p("^=")
                    |str_p("|=")
                    |str_p("&&")
                    |str_p("||")
                    |str_p("%=")
                    |str_p("[]")
                    |str_p("()")
                    |str_p("->")
                    |ch_p('+')
                    |'-'
                    |'*'
                    |'/'
                    |'='
                    |'<'
                    |'>'
                    |'%'
                    |'&'
                    |'^'
                    |'!'
                    |'|'
                    |'~'
                    |',');

            templateDefinition = 
                str_p( "template" ) >> comment_nest_p("<",">") 
                >> (((str_p("struct")|str_p("class")) 
                    >> lexeme_d[+(alnum_p| '_')])
                    |(lexeme_d[+(alnum_p| '_' | ':' )] 
                    >> !comment_nest_p("<",">") >> !*(str_p("const")|'*'|'&') 
                >> operatorDefinition | lexeme_d[+(alnum_p|'_')]
                >> comment_nest_p('(',')') >> !str_p( "const" ))) 
                >> comment_nest_p( "{", "}" ) 
                >> !ch_p(';'); // won't handle forward decls

            staticDefinition = 
                str_p( "static" ) >> lexeme_d[ +(~ch_p(';')) ] >> ';';

            enumDefinition = 
                str_p( "enum" ) >> !lexeme_d[+(alnum_p| '_')] 
                >> comment_nest_p( "{","}" ) >> ';';

            declaration = 
                typedefDefinition | 
                templateDefinition | 
                staticDefinition | 
                enumDefinition;

            function = 
                lexeme_d[+(alnum_p| '_' | ':' )] >> !comment_nest_p('<','>') 
                    >> !*(str_p("const")|'*'|'&') 
                    >> ( operatorDefinition | lexeme_d[+(alnum_p|'_')] )
                    >> comment_nest_p('(',')') >> !str_p( "const" ) 
                    >> !(str_p("throw") >> comment_nest_p("(", ")"))
                    >> (comment_nest_p( "{", "}" ) | ';');

            constructor = 
                lexeme_d[+(alnum_p| '_' | '~' )] >> comment_nest_p('(',')') 
                >> !(':' >> lexeme_d[+(alnum_p| '_')] >> comment_nest_p('(',')') 
                >> *(ch_p(',') >> lexeme_d[+(alnum_p| '_')] 
                >> comment_nest_p('(',')'))) >> (comment_nest_p("{", "}")|';');

            memberType = 
                ( str_p( "enum" ) >> !lexeme_d[+(alnum_p| '_')] 
                    >> comment_nest_p( "{","}" ) 
                | (((str_p("unsigned") >> !str_p("long"))|str_p("long")) 
                    >> (str_p("int")|str_p("char")|str_p("long")) ) 
                | ( lexeme_d[+(alnum_p| '_' | ':' )] 
                >> !comment_nest_p('<','>') ) ) 
                >> !(str_p("const")|'*'|'&');

            memberName = 
                lexeme_d[+(alnum_p|'_')]
                    [store_field_a(
                        phoenix::var(self.hash),
                       phoenix::var(self.structName),
                        construct_<std::string>(arg1,arg2))];

            member = 
                memberType
                >> memberName >> !comment_nest_p('[',']') 
                >> *(ch_p(',') >> !ch_p('*') 
                    >> memberName 
                    >> !comment_nest_p('[',']')) 
                >> ';';

            item = declaration | function | constructor | member;

            structDefinition = 
                str_p( "struct" ) 
                >> ( +( alnum_p | '_' ) )[assign_a(self.structName)] 
                >> '{' 
                >> *item 
                >> '}' >> ';';

            header = *( structDefinition | anychar_p );
        }

        rule<ScannerT> header, structDefinition, item;
        rule<ScannerT> declaration, function, constructor, member;
        rule<ScannerT> typedefDefinition, templateDefinition;
        rule<ScannerT> staticDefinition, enumDefinition;
        rule<ScannerT> operatorDefinition;
        rule<ScannerT> memberType, memberName;

        rule<ScannerT> const& start() const { return header; }
    };

    mutable map< string,vector<string> > hash;  // hack: store state better
    mutable string structName;
};
map< string,vector<string> > parseHeader( const char* filename ) {
    //
    // Do not use file_iterator<> as it crashes on HP-UX
    //
    string content = slurpFile( filename );

    CppGrammar grammar;

    bool const headerParsed = parse( 
        content.begin(),
        content.end(),
        grammar >> end_p, 
        space_p | comment_p( "//" ) | comment_p( "/*", "*/" ) ).full;

    if( !headerParsed ) {
        throw ParseException();
    }

    return grammar.hash;
}

void showUsage( const char* programName ) {
    cerr << "Usage: " << programName << " [--includes <include_list|@include.inc>] <src|header> <headerfile.h> <--testbed|--normal>" << endl; 
}

char const* basename( const char* str, char delim ) {
    char const* result = strrchr( str, delim );
    return result ? result + 1 : str;
}

void printSerializeSource( HeaderContext const& context ) {
#ifdef SV_UNIX
    const char* filename = basename( context.filename.c_str(), '/' );
#else
	const char* filename = basename( context.filename.c_str(), '\\' );
#endif
    string cppName = filename;
    const char* filenameStr = cppName.c_str();
    if( filenameStr == strstr( filenameStr, "intermediate_" ) ) {
        cppName = cppName.substr( sizeof( "intermediate_" ) - 1 );
    }
    else {
        cppName[ cppName.length() -1 ] = 'c';
        cppName += "pp";
    }

    cout << "//" << endl
         << "// serialize" << cppName << ": (generated file) marshal/unmarshal code" << endl
         << "//" << endl
         << endl;
    for( vector<string>::const_iterator i( context.includes.begin() ), end( context.includes.end() ); i != end; ++i ) {

        // If using gcc 3.3.3 (SuSe's default), because compiler refuses to
        // compile header files (even when using -MM), we rename as
        // intermediate<filename>.cpp. So we undo that hack here.
        string finalName = *i;
        const char* includeFilename = finalName.c_str();
        if( includeFilename == strstr( includeFilename, "intermediate_" ) ) {
            finalName = finalName.substr( sizeof( "intermediate_" ) - 1 );
            finalName = finalName.substr( 0, finalName.size() - 3 );
            finalName += "h";
        }

        cout << "#include \"serialize" << finalName << "\"" << endl;
    }
    if (enableDebug) {
        cout << "using namespace std;" << endl;
    }

    cout << endl;
    for( map< string,vector<string> >::const_iterator i( context.info.begin() ),
        end( context.info.end() ); i != end; ++i )
    {
        cout << "template<>" << endl
             << (*i).first << " unmarshal<" << (*i).first << ">( UnmarshalStream& in, int version ) {" << endl;
        cout << "    " << (*i).first << " s;" << endl 
             << endl;
		if (enableDebug) {
			cout << "    cout << endl;" << endl;
			cout << "    tabs++;" << endl;
			cout << "    cout.width(tabs*TAB_SPACE+MAX_WORD_LEN);" << endl;
			cout << "    cout << \"" << (*i).first << "\" << endl;" << endl;
			cout << "    tabs++;" << endl;
		}
        for( vector<string>::const_iterator j( (*i).second.begin() ), endJ( (*i).second.end() ); j != endJ; ++j ) {
			if (enableDebug) {
				cout << "    cout.width(tabs*TAB_SPACE+MAX_WORD_LEN);" << endl;
				cout << "    cout << \"" << *j << "\";" << endl;
			}
			cout << "    in >> s." << *j << ";" << endl;
			if (enableDebug) {
				cout << "    cout << endl;" <<  endl;
			}

        }

		if (enableDebug) {
			cout << "    tabs--;" << endl;
			cout << "    tabs--;" << endl;
			cout << "    cout.width(tabs*TAB_SPACE+MAX_WORD_LEN);" << endl; 
		}
		cout << endl
             << "    return s;" << endl
             << "}" << endl
             << endl;

        cout << "std::ostream& operator<<( std::ostream& o, CxArgObj<" << (*i).first << "> v ) {" << endl
             << "     " << (*i).first << " const& x = v.value;" << endl
             << endl
             << "    o << " << "\"a:" << static_cast<unsigned>( (*i).second.size()+1 ) << ":{i:0;i:0;\";" << endl;
        int index = 1;
        for( vector<string>::const_iterator j( (*i).second.begin() ), end( (*i).second.end() ); j != end; ++j, ++index ) {
             cout << "    o << \"i:" << index << ";\" << cxArg( x." << *j << " );" << endl;
        }
        cout << "    o << \"}\";" << endl
             << endl
             << "    return o;" << endl
             << "}" << endl
             << endl;
    }
}

void printSerializeHeader( HeaderContext const& context ) {
#ifdef SV_UNIX
    const char* filename = basename( context.filename.c_str(), '/' );
#else
	const char* filename = basename( context.filename.c_str(), '\\' );
#endif
    string tempFilename;
    if( filename == strstr( filename, "intermediate_" ) ) {
        filename = filename + sizeof( "intermediate_" ) - 1;
        tempFilename = filename;
        tempFilename = tempFilename.substr( 0, tempFilename.size() - 3 );
        tempFilename += 'h';
        filename = tempFilename.c_str();
    }

    string headerGuard = filename;
    headerGuard[ headerGuard.find('.') ] = '_';
    transform( headerGuard.begin(), headerGuard.end(), headerGuard.begin(), static_cast<int(*)(int)>( toupper ) );

    cout << "//" << endl
         << "// serialize" << filename << ": (generated file) " << "marshaling/unmarshaling code" << endl
         << "//" << endl
         << "#ifndef SERIALIZE" << headerGuard << endl
         << "#define SERIALIZE" << headerGuard << endl
         << endl
         << "#include \"unmarshal.h\"" << endl
         << "#include \"marshal.h\"" << endl
         << "#include \"" << filename << "\"" << endl
         << endl;

    for( map< string,vector<string> >::const_iterator i( context.info.begin() ),
        end( context.info.end() ); i != end; ++i )
    {
        cout << "template<>" << endl
             << "struct UnmarshalIsStructure<" << (*i).first << "> {" << endl
             << "    static const bool Value = true;" << endl
             << "};" << endl
             << endl
             << "template<>" << endl
             << (*i).first << " unmarshal<" << (*i).first << ">( UnmarshalStream& in, int version );" << endl
             << endl
             << "std::ostream& operator<<( std::ostream& o, CxArgObj<" << (*i).first << "> v );" << endl
             << endl;
    }
    cout << "#endif // SERIALIZE" << headerGuard << endl;
}

void printXmlizeSource( HeaderContext const& context ) {
#ifdef SV_UNIX
    const char* filename = basename( context.filename.c_str(), '/' );
#else
	const char* filename = basename( context.filename.c_str(), '\\' );
#endif
    string cppName = filename;
    const char* filenameStr = cppName.c_str();
    if( filenameStr == strstr( filenameStr, "intermediate_" ) ) {
        cppName = cppName.substr( sizeof( "intermediate_" ) - 1 );
    }
    else {
        cppName[ cppName.length() -1 ] = 'c';
        cppName += "pp";
    }

    cout << "//" << endl
         << "// xmlize" << cppName << ": (generated file) xmlmarshal/xmlunmarshal code" << endl
         << "//" << endl
         << endl;
    for( vector<string>::const_iterator i( context.includes.begin() ), end( context.includes.end() ); i != end; ++i ) {

        // If using gcc 3.3.3 (SuSe's default), because compiler refuses to
        // compile header files (even when using -MM), we rename as
        // intermediate<filename>.cpp. So we undo that hack here.
        string finalName = *i;
        const char* includeFilename = finalName.c_str();
        if( includeFilename == strstr( includeFilename, "intermediate_" ) ) {
            finalName = finalName.substr( sizeof( "intermediate_" ) - 1 );
            finalName = finalName.substr( 0, finalName.size() - 3 );
            finalName += "h";
        }

        cout << "#include \"xmlize" << finalName << "\"" << endl;
    }
    cout << endl;
    for( map< string,vector<string> >::const_iterator i( context.info.begin() ),
        end( context.info.end() ); i != end; ++i )
    {
        cout << "void extractfromreqresp( const ParameterGroup& o, " << (*i).first << " &x, const std::string &name ) {" << endl
             << "     " << "const ParameterGroup *ppg = &o;" << endl
             << "     " << "if (!name.empty()) {" << endl
             << "     " << "    " << "ParameterGroups_t::const_iterator i = o.m_ParamGroups.find(name);" << endl
             << "     " << "    " << "if (i != o.m_ParamGroups.end()) {" << endl
             << "     " << "    " << "    " << "const ParameterGroup &pg = i->second;" << endl
             << "     " << "    " << "    " << "ppg = &pg;" << endl
             << "     " << "    " << "}" << endl
             << "     " << "    " << "else {" << endl
             << "     " << "    " << "    " << "throw INMAGE_EX(\"could not find expected parameter name:\")(name);" << endl
             << "     " << "    " << "}" << endl
             << "     " << "}" << endl
             << endl;
        for( vector<string>::const_iterator j( (*i).second.begin() ), end( (*i).second.end() ); j != end; ++j ) {
             cout << "     extractfromreqresp(*ppg, x." << *j << ", " << "\"" << *j << "\"" << " );" << endl;
        }
        cout << endl
             << "}" << endl
             << endl;

        cout << "void insertintoreqresp( ParameterGroup& o, CxArgObj<" << (*i).first << "> v, const std::string &name ) {" << endl
             << "     " << "ParameterGroup *ppg = &o;" << endl
             << "     " << "if (!name.empty()) {" << endl
             << "     " << "    " <<"std::pair<ParameterGroupsIter_t, bool> pgpair = o.m_ParamGroups.insert(std::make_pair(name, ParameterGroup()));" 
             << endl
             << "     " << "    " << "ParameterGroupsIter_t &pgpairiter = pgpair.first;" << endl
             << "     " << "    " << "ParameterGroup &pg = pgpairiter->second;" << endl
             << "     " << "    " << "ppg = &pg;" << endl
             << "     " << "}" << endl
             << endl
             << "     " << (*i).first << " const& x = v.value;" << endl;
        for( vector<string>::const_iterator j( (*i).second.begin() ), end( (*i).second.end() ); j != end; ++j ) {
             cout << "     insertintoreqresp(*ppg, cxArg( x." << *j << " )" << ", " << "\"" << *j << "\"" << " );" << endl;
        }
        cout << endl
             << "}" << endl
             << endl;
    }
    if (enableDebug) {
        cout << "using namespace std;" << endl;
    }
}

void printXmlizeHeader( HeaderContext const& context ) {
#ifdef SV_UNIX
    const char* filename = basename( context.filename.c_str(), '/' );
#else
	const char* filename = basename( context.filename.c_str(), '\\' );
#endif
    string tempFilename;
    if( filename == strstr( filename, "intermediate_" ) ) {
        filename = filename + sizeof( "intermediate_" ) - 1;
        tempFilename = filename;
        tempFilename = tempFilename.substr( 0, tempFilename.size() - 3 );
        tempFilename += 'h';
        filename = tempFilename.c_str();
    }

    string headerGuard = filename;
    headerGuard[ headerGuard.find('.') ] = '_';
    transform( headerGuard.begin(), headerGuard.end(), headerGuard.begin(), static_cast<int(*)(int)>( toupper ) );

    cout << "//" << endl
         << "// xmlize" << filename << ": (generated file) " << "xmlmarshaling/xmlunmarshaling code" << endl
         << "//" << endl
         << "#ifndef XMLIZE" << headerGuard << endl
         << "#define XMLIZE" << headerGuard << endl
         << endl
         << "#include \"xmlunmarshal.h\"" << endl
         << "#include \"xmlmarshal.h\"" << endl
         << "#include \"" << filename << "\"" << endl
         << endl;


    cout << endl;
    for( map< string,vector<string> >::const_iterator i( context.info.begin() ),
        end( context.info.end() ); i != end; ++i )
    {
        cout << "void insertintoreqresp( ParameterGroup& o, CxArgObj<" << (*i).first << "> v, const std::string &name = std::string() );" << endl << endl;
        cout << "void extractfromreqresp( const ParameterGroup& o, " << (*i).first << " &x, const std::string &name = std::string() );" << endl << endl;
  
        cout << "namespace NS_" << (*i).first << endl
             << '{' << endl;
        for( vector<string>::const_iterator j( (*i).second.begin() ), end( (*i).second.end() ); j != end; ++j ) {
             cout << "    const char " << *j << "[] = " << "\"" << *j << "\"" << ";" << endl;
        }
        cout << '}' << endl << endl;
    }

    cout << "#endif // XMLIZE" << headerGuard << endl;
}

void trimEndOfLine( std::string& s ) {
    size_t pos = s.size();
    for( ; pos && ( '\r' == s[pos-1] || '\n' == s[pos-1] ); --pos ) ;
    s.erase( pos, s.size() - pos );
}

vector<string> parseIncludes( const char* str ) {
   vector<string> result;

   // Windows style includes specified by --includes @filename.inc 
   if( '@' == str[0] ) {
       ifstream in( &str[1] );
       if( !in ) {
           cerr << "couldn't read from " << &str[1] << endl;
           return result;
       }

       // Process line by line rather than parsing
       string line;
       getline( in, line );
       trimEndOfLine( line );
       result.push_back( line );

       for( getline( in, line ); !in.eof(); getline( in, line ) ) {
           trimEndOfLine( line );
           const char prefix[] = "Note: including file: ";
           if( 0 == line.rfind( prefix ) && ' ' != line[sizeof(prefix)-1] ) {
               const char* includeFile = strrchr( line.c_str(), '\\' );
               if( NULL != includeFile && strstr( includeFile-7, "\\config" ) == includeFile-7 ) {
                   result.push_back( &includeFile[1] );
               }
           }
       }
       return result;
   }

   // Unix style includes
   if( !parse( str, 
       +(alnum_p|'_'|'-') >> ".o:" >> !ch_p( '\\' )
           >> *(((str_p("config/") 
           >> lexeme_d[+(alnum_p|'_'|'/'|'.'|'-'|'+')][push_back_a(result)]) |
               ( lexeme_d[+(alnum_p|'_'|'/'|'.'|'-'|'+')] ) )
           >> !ch_p('\\') ),
       space_p | comment_p( "#" ) ).full )
    {
        cerr << "Couldn't parse argument to `includes': " << str << endl;
    }

   return result;
}

int main( int argc, const char* argv[] ) {

    init_inm_safe_c_apis();
    const char* program = argv[0];
    const std::string XMLIZE = "xmlize";
    const std::string SERIALIZE = "serialize";

    if( argc < 3 ) {
        showUsage( program );
        return -1;
    }

    HeaderContext context;
    const char* action = argv[1];
    context.filename   = argv[2];
    vector<string> includes;
    if( string(argv[1]) == "--includes" ) {
        if( 7 != argc ) {
            showUsage( program );
            return -1;
        }
        action = argv[3];
        context.filename = argv[4];
        context.includes = parseIncludes( argv[2] );
		if( string(argv[5]) == "--normal" ) {
			enableDebug = false;
		}
		else if ( string(argv[5]) == "--testbed" ) {
			enableDebug = true;
		}
    }

    OutputDispatcher_t serialdispatch;
    serialdispatch["src"] = printSerializeSource;
    serialdispatch["header"] = printSerializeHeader;

    OutputDispatcher_t xmldispatch;
    xmldispatch["src"] = printXmlizeSource;
    xmldispatch["header"] = printXmlizeHeader;

    Printer_t func = serialdispatch[ action ];
    if (XMLIZE == argv[6])
    {
        func = xmldispatch[ action ];
    }

    if( 0 == func ) {
        showUsage( program );
        return -1;
    }

    try {
        context.info = parseHeader( context.filename.c_str() );
    }
    catch( std::exception const& e ) {
        cerr << program << ": exception " << e.what() << endl;
        return -1;
    }

    (*func)( context );

    return 0;
}

