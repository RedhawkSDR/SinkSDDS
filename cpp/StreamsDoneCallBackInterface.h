/*
 * StreamsDoneCallBackInterface.h
 *
 *  Created on: Jul 1, 2016
 *      Author: 
 */

#ifndef STREAMSDONECALLBACKINTERFACE_H_
#define STREAMSDONECALLBACKINTERFACE_H_


class StreamsDoneCallBackInterface {
public:
    virtual void streamsDone(std::string streamId) = 0;
};




#endif /* STREAMSDONECALLBACKINTERFACE_H_ */
