#include <fstream>
#include <string>
#include <map>
#include <new>
#include <cctype>
#include <utility>
#include <sstream>
#include <assert.h>
#include "svtypes.h"
#include "svconfig.h"
#include "inmsafecapis.h"

using namespace std;

SVConfig* SVConfig::pInstance = 0;

struct LineParser
{
    enum ParseError { CONFIG_PARSE_OK, CONFIG_PARSE_INVALID_NAME, CONFIG_MISSING_VALUE, 
        CONFIG_PARSE_INVALID_QUOTING, CONFIG_PARSE_INVALID_SECTION_NAME, CONFIG_PARSE_INVALID_SYNTAX };

    LineParser( string line );
    bool iskeychar( char ch ) const;
    ParseError rc;
    SVConfig::ParsedLine result;
    typedef void (LineParser::*State)( char ch );
    State s;

    void whitespace1( char ch );
    void name( char ch );
    void whitespace2( char ch );
    void whitespace3( char ch );
    void value( char ch );
    void whitespace4( char ch );
    void quote( char ch );
    void quote2( char ch );
    void whitespace5( char ch );
    void whitespace6( char ch );
    void section( char ch );
    void whitespace7( char ch );
    void comment( char ch );
    void invalid( char ch );
};


//
// Convert a string to lowercase.
//
static void lowercase(string& s) 
{
    for (size_t i = 0, sz = s.size(); i < sz; i++)
    s[i] = tolower(s[i]);
}

SVConfig*
SVConfig::GetInstance()
{
    static SVConfig instance;

    if( NULL == pInstance )
    {
        pInstance = &instance;
    }

    return pInstance;
}

SVConfig::SVConfig()
{
}

SVConfig::~SVConfig()
{
    Clear();
}

SVERROR 
SVConfig::Parse(string const& configFile) 
{
    configFilePath = configFile;
    ifstream file(configFilePath.c_str());
    if(!file) return SVE_FILE_NOT_FOUND;

    return ParseStream( file );
}

SVERROR 
SVConfig::ParseStream( istream& o )
{
    SVERROR rc;

    Clear();

    string curSection;
    string line;

    while (getline(o,line)) 
    {
        LineParser parser( line );

        if( LineParser::CONFIG_PARSE_OK != parser.rc )
        {
            return( SVE_INVALID_FORMAT );
        }
        else
        {
            if( parser.result.name != "" )
            {
                rc = SetValue( curSection, parser.result.name, parser.result.value );
                if( rc.failed() )
                {
                    return( rc );
                }
            }
            else if( parser.result.sectionName != "" )
            {
                string sectionName = parser.result.sectionName;

                curSection = CreateSection( sectionName );
            }
        }
    }

    return( rc );
}


SVERROR 
SVConfig::ParseString(std::string const& configStr)
{
    stringstream stream( configStr );
    return( ParseStream( stream ) );
}


bool LineParser::iskeychar( char ch ) const
{
    return( isalnum( ch ) || '_' == ch );
}

LineParser::LineParser( std::string line )
{
    rc = CONFIG_PARSE_OK;
    result.quoteChar = 0;
    s = &LineParser::whitespace1;

    for( string::const_iterator i=line.begin(); i != line.end(); ++i )
    {
        char ch = *i;
        (this->*s)( ch );
    }

    //
    // If we're not in a terminal state, return an error
    //
    if( &LineParser::name == s || &LineParser::whitespace2 == s )
    {
        rc = CONFIG_MISSING_VALUE;
    }
    else if( &LineParser::quote == s || &LineParser::quote2 == s )
    {
        rc = CONFIG_PARSE_INVALID_QUOTING;
    }
    else if( &LineParser::whitespace6 == s || &LineParser::section == s || &LineParser::whitespace7 == s )
    {
        rc = CONFIG_PARSE_INVALID_SYNTAX;
    }

    if( CONFIG_PARSE_OK == rc )
    {
        assert( &LineParser::invalid != s );
    }
}

//
// Whitespace before name or section bracket
//
void LineParser::whitespace1( char ch )
{
    if( isspace( ch ) )
    {
        result.spaceLeadingName += ch;
    }
    else if( '[' == ch ) 
    {
        s = &LineParser::whitespace6;
    }
    else if( iskeychar( ch ) )
    {
        result.name += ch;
        s = &LineParser::name;
    }
    else if( '#' == ch )
    {
        s = &LineParser::comment;
    }
    else
    {
        rc = CONFIG_PARSE_INVALID_NAME;
        s = &LineParser::invalid;
    }
}

//
// Whitespace after name, before equals
//
void LineParser::whitespace2( char ch )
{
    if( isspace( ch ) )
    {
        result.spaceTrailingName += ch;
    }
    else if( '=' == ch )
    {
        s = &LineParser::whitespace3;
    }
    else
    {
        rc = CONFIG_PARSE_INVALID_NAME;
        s = &LineParser::invalid;
    }
}

//
// Whitespace after '=', before start of value
//
void LineParser::whitespace3( char ch ) 
{
    if( isspace( ch ) )
    {
        result.spaceLeadingValue += ch;
    }
    else if( '"' == ch || '\'' == ch )
    {
        result.quoteChar = ch;
        s = &LineParser::quote;
    }
    else
    {
        result.value += ch;
        s = &LineParser::value;
    }
}

//
// Whitespace in value, before optional comment
//
void LineParser::whitespace4( char ch )
{
    if( isspace( ch ) )
    {
        result.spaceTrailingValue += ch;
    }
    else if( '#' == ch )
    {
        s = &LineParser::comment;
    }
    else
    {
        result.value += result.spaceTrailingValue;
        result.value += ch;
        s = &LineParser::value;
    }
}

//
// Whitespace after end of quoted value
//
void LineParser::whitespace5( char ch )
{
    if( isspace( ch ) )
    {
        result.spaceTrailingValue += ch;
    }
    else if( '#' == ch )
    {
        s = &LineParser::comment;
    }
    else
    {
        rc = CONFIG_PARSE_INVALID_SYNTAX;
        s = &LineParser::invalid;
    }
}

//
// Whitespace after '[' for section
//
void LineParser::whitespace6( char ch )
{
    if( isspace( ch ) )
    {
        result.spaceLeadingValue += ch;
    }
    else if( iskeychar( ch ) )
    {
        result.sectionName += ch;
        s = &LineParser::section;
    }
    else
    {
        rc = CONFIG_PARSE_INVALID_SECTION_NAME;
        s = &LineParser::invalid;
    }
}

//
// Whitespace after section name, before ']'
//
void LineParser::whitespace7( char ch )
{
    if( isspace( ch ) )
    {
        result.spaceTrailingName += ch;
    }
    else if( ']' == ch )
    {
        s = &LineParser::whitespace5;
    }
    else
    {
        rc = CONFIG_PARSE_INVALID_SECTION_NAME;
        s = &LineParser::invalid;
    }
}

//
// Get `name'
//
void LineParser::name( char ch )
{
    if( iskeychar( ch ) )
    {
        result.name += ch;
    }
    else if( isspace( ch ) )
    {
        s = &LineParser::whitespace2;
    }
    else if( '=' == ch )
    {
        s = &LineParser::whitespace3;
    }
    else
    {
        rc = CONFIG_PARSE_INVALID_NAME;
        s = &LineParser::invalid;
    }
}

//
// Get unquoted value
//
void LineParser::value( char ch )
{
    if( isspace( ch ) )
    {
        result.spaceTrailingValue += ch;
        s = &LineParser::whitespace4;
    }
    else
    {
        result.value += ch;
    }
}

//
// Get quoted string
//
void LineParser::quote( char ch )
{
    if( ch == result.quoteChar )
    {
        s = &LineParser::whitespace5;
    }
    else if( '\\' == ch )
    {
        s = &LineParser::quote2;
    }
    else
    {
        result.value += ch;
    }
}


//
// Handle backslash escape characters
//
void LineParser::quote2( char ch )
{
    if( 't' == ch )
    {
        result.value += '\t';
    }
    else if( 'r' == ch )
    {
        result.value += '\r';
    }
    else if( 'n' == ch )
    {
        result.value += '\n';
    }
    else
    {
        result.value += ch;
    }
    s = &LineParser::quote;
}

//
// Get section name
//
void LineParser::section( char ch )
{
    if( iskeychar( ch ) || '.' == ch )
    {
        result.sectionName += ch;
    }
    else if( ']' == ch )
    {
        s = &LineParser::whitespace5;
    }
    else if( isspace( ch ) )
    {
        result.spaceTrailingName += ch;
        s = &LineParser::whitespace7;
    }
    else
    {
        rc = CONFIG_PARSE_INVALID_SECTION_NAME;
        s = &LineParser::invalid;
    }
}

//
// Get comment
//
void LineParser::comment( char ch )
{
    result.comment += ch;
}

//
// Invalid state just discards characters
//
void LineParser::invalid( char ch )
{
}

SVERROR 
SVConfig::GetValue(string const& entry, string& value) const 
{
    map<string,string>::const_iterator ci = config.find(entry);
    if (ci == config.end()) return SVE_FAIL;
    value = ci->second;
    return SVS_OK;
}

SVERROR
SVConfig::UpdateConfigParameter(string const& key, string const& value) 
{
    config[key] = value;
    return SVS_OK;
}

SVERROR
SVConfig::Write() 
{
    ofstream file(configFilePath.c_str());
    if (!file) return SVE_FAIL;
    map<string,string>::const_iterator ci;
    string key;
    for (ci=config.begin(); ci != config.end(); ++ci) 
    {
        key = ci->first;
        file << key << " = " << ci->second << '\n';
    }
    file.flush();
    file.close();
    return SVS_OK;
}

int  
SVConfig::GetTotalNumberOfKeys()
{
    return (int) config.size();
}

SVERROR
SVConfig::WriteKeyValuePairToFile(string const& key, string const& value)
{
    ofstream svstream;
    svstream.open(configFilePath.c_str(),ios::out | ios::app );
    svstream << '\n' << key << " = " << value;
    svstream.flush();
    svstream.close();
    return SVS_OK;
}

SVERROR 
SVConfig::GetConfigFileName(string& name) const
{
    name = configFilePath;
	return SVS_OK;
}

void 
SVConfig::SetConfigFileName(const char* fileName)
{
    configFilePath = fileName;
}

void SVConfig::Clear()
{
    config.clear();
    m_arraycount.clear();
    m_isSection.clear();
    m_lines.clear();    
}

void SVConfig::RenameSectionIntoArrayEntry( string oldSectionName )
{
    vector< pair<string,string> > updates;

    for( map<string,string>::iterator i = config.begin(); i != config.end(); ++i )
    {
        string key = i->first;
        if( key.substr( 0, oldSectionName.length()+1 ) == oldSectionName + "." )
        {
            string value = config[ key ];
            updates.push_back( make_pair( key, value ) );
        }
    }

    for( vector< pair<string,string> >::iterator j = updates.begin(); j != updates.end(); ++j )
    {
        string key = j->first;
        config.erase( key );
        key.replace( 0, oldSectionName.length(), oldSectionName + "[0]" );
        config[ key ] = j->second;
    }
}

string SVConfig::CreateSection( std::string const& sectionName )
{
    string curSection;

    map<string,string>::const_iterator entry = config.find( sectionName );
    if( config.end() == entry )
    {
        map<string,int>::const_iterator arrayEntry = m_arraycount.find( sectionName );
        if( m_arraycount.end() == arrayEntry )
        {
            map<string,bool>::const_iterator sectionEntry = m_isSection.find( sectionName );
            if( m_isSection.end() == sectionEntry )
            {
                //
                // Key did not exist; create section
                //
                curSection = sectionName;
                //CreateSection( curSection );
                m_isSection[ sectionName ] = true;

                //
                // Dotted section names necessarily imply their parent sections
                //
                string::size_type pos = 0;
                while( true )
                {
                    pos = sectionName.find_first_of( '.', pos );
                    if( string::npos == pos )
                    {
                        break;
                    }
                    m_isSection[ sectionName.substr( 0, pos ) ] = true;
                    pos ++;
                }
            }
            else
            {
                //
                // Rename existing section
                //
                m_isSection.erase( sectionName );           // delete old section name
                m_isSection[ sectionName+"[0]" ] = true;    // add section name[0]
                RenameSectionIntoArrayEntry( sectionName ); // rename all old section variables to section[0]
                curSection = sectionName + "[1]";
                //config[ curSection ] = parser.result.value; // set array value for name[1]
                m_arraycount[ sectionName ] = 2;            // add array for name
            }
        }
        else
        {
            //
            // Add section to existing array
            //
            int arrayCount = arrayEntry->second;
            char szNumber[ 16 ];
			inm_sprintf_s(szNumber, ARRAYSIZE(szNumber), "[%d]", arrayCount);
            curSection = sectionName + szNumber;
            m_isSection[ curSection ] = true;
            m_arraycount[ sectionName ] = arrayCount + 1;
        }
    }
    else
    {
        //
        // Rename existing value into array element
        //
        string value0 = entry->second;
        config.erase( sectionName );
        
        config[ sectionName + "[0]" ] = value0;
        m_arraycount[ sectionName ] = 2;
    }

    return( curSection );
}

SVERROR SVConfig::SetValue( std::string const& sectionName, std::string const& name, std::string const& value )
{
    SVERROR rc;
    string keyName = (sectionName == "") ? name : sectionName + "." + name;

    map<string,string>::const_iterator entry = config.find( keyName );
    if( config.end() == entry )
    {
        map<string,int>::const_iterator arrayEntry = m_arraycount.find( keyName );
        if( m_arraycount.end() == arrayEntry )
        {
            map<string,bool>::const_iterator sectionEntry = m_isSection.find( keyName );

            if( m_isSection.end() == sectionEntry )
            {
                //
                // Key did not exist; create simple name=value
                //
                config[ keyName ] = value;
            }
            else
            {
                //
                // Rename old section variables to section[ 0 ]
                //                            
                m_isSection.erase( keyName );           // delete old section name                            
                m_isSection[ keyName+"[0]" ] = true;    // add section name[0]                            
                RenameSectionIntoArrayEntry( keyName ); // rename all old section variables to section[0]                            
                config[ keyName+"[1]" ] = value;        // set array value for name[1]                            
                m_arraycount[ keyName ] = 2;            // add array for name
            }
        }
        else
        {
            //
            // Key is an array; append to the existing array
            //
            int arrayCount = arrayEntry->second;
            char szNumber[ 16 ];
			inm_sprintf_s(szNumber, ARRAYSIZE(szNumber), "[%d]", arrayCount);
            config[ keyName + szNumber ] = value;
            m_arraycount[ keyName ] = arrayCount + 1;
        }
    }
    else
    {
        //
        // Duplicate instance of keyName -> keyName is an array
        //
        string value0 = entry->second;
        config.erase( keyName );
        
        config[ keyName + "[0]" ] = value0;
        config[ keyName + "[1]" ] = value;
        m_arraycount[ keyName ] = 2;
    }

    return( rc );
}


int SVConfig::GetArraySize( std::string const& name )
{
    map<string,int>::const_iterator arrayEntry = m_arraycount.find( name );
    return( m_arraycount.end() == arrayEntry ) ? 0 : arrayEntry->second;
}
