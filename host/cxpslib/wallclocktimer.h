
///
/// \file wallclocktimer.h
///
/// \brief simple wall clock timer acurate to milli seconds
///

#ifndef WALLCLOCKTIMER_H
#define WALLCLOCKTIMER_H

#include <sys/timeb.h>

/// \brief timer for timing wall clock durations
class WallClockTimer {
public:
    WallClockTimer() {	
        m_startTime.time = 0;
        m_startTime.millitm = 0;       
        m_duration = 0;        
    }

    /// \brief start the timer
    ///
    /// \note
    /// \li \c Calling start consecutive times with out calling stop will not
    /// restart the time unsless restart is true (default is false)
    /// \li \c Calling any of the elapsed time functions after calling start
    /// but with out calling stop results in 0 elapsed time
    void start(bool restart = false) ///< indicates if the timer should be restarted if already started true: yes, false: no
        {
            m_duration = 0;
            if (restart || 0 == m_startTime.time) {
                ftime(&m_startTime);
            }
        }

    /// \brief stop the timer
    ///
    /// \note
    /// \li \c it is safe to call stop before calling start.
    /// \li \c Calling any of the elapsed time functions with out having called start and then stop
    /// results in 0 elapsed time
    void stop() {
        if (m_startTime.time > 0) {
            struct timeb endTime;             
            ftime(&endTime);
            m_duration = static_cast<long double>((endTime.time * 1000 + endTime.millitm) - (m_startTime.time * 1000 + m_startTime.millitm));
            m_startTime.time = 0;
            m_startTime.millitm = 0;
        }
    }

    /// \brief get elasped time in hours
    long double elapsedHours() {
        return m_duration / (1000 * 60 * 60);
    }

    /// \brief get elasped time in minutes
    long double elapsedMinutes() {
        return m_duration / (1000 * 60);
    }

    /// \brief get elasped time in seconds   
    long double elapsedSeconds() {
        return m_duration / 1000;
    }

    /// \brief get elasped time in milli seconds
    long double elapsedMilliSeconds() {
        return m_duration;
    }

protected:

private:
    struct timeb m_startTime; ///< holds the start time
    long double m_duration;   ///< holds the duration in milli seconds
};

#endif // ifndef WALLCLOCKTIMER_H
