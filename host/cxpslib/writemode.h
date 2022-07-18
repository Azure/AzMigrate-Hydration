
///
/// \file writemode.h
///
/// \brief
///

#ifndef WRITEMODE_H
#define WRITEMODE_H

/// \brief file wrtie modes
enum WRITE_MODES {
    WRITE_MODE_NORMAL,  ///< use buffered I/O system does flush to disk
    WRITE_MODE_FLUSH    ///< use bufferd I/O manual flush to disk after last write
};

#endif // WRITEMODE_H
