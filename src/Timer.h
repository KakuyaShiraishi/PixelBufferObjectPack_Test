#ifndef TIMER_H_DEF
#define TIMER_H_DEF

#if defined(WIN32) || defined(_WIN32)   
#include <windows.h>
#else        
#include <sys/time.h>
#endif


class Timer
{
public:
    Timer();                                   
    ~Timer();                                 

    void   start();                           
    void   stop();                          
    double getElapsedTime();                 
    double getElapsedTimeInSec();             
    double getElapsedTimeInMilliSec();        
    double getElapsedTimeInMicroSec();         


protected:


private:
    double startTimeInMicroSec;               
    double endTimeInMicroSec;                  
    int    stopped;                            
#if defined(WIN32) || defined(_WIN32)
    LARGE_INTEGER frequency;                   
    LARGE_INTEGER startCount;                   
    LARGE_INTEGER endCount;                     
#else
    timeval startCount;                         
    timeval endCount;                          
#endif
};

#endif 
