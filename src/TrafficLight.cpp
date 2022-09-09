#include <iostream>
#include <random>
#include "TrafficLight.h"


/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 

    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this] { return !_queue.empty(); });
    T msg = std::move(_queue.front());
    _queue.pop_front();
    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    std::lock_guard<std::mutex> lockGuard(_mutex);
    _queue.clear();  // flushes queue for avoiding accumulations of messages
    _queue.push_back( std::move(msg) );
    _cond.notify_one();  // notify client after pushing msg into deque
}


/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _messageQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
    _currentPhase = TrafficLightPhase::green;
}
TrafficLight::~TrafficLight()
{
    // nothing to do here
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while(true)
    {
        // goes indirectly in wait state
        if( _messageQueue->receive() == TrafficLightPhase::green)
        {
            return;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase() const
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this ));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

    // make random facility
    std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distrib(4000,6000);

    std::chrono::time_point<std::chrono::system_clock> lastUpdate;
 
    // the endless traffic light cycle
    while(true)
    {
        long cycleDuration = distrib(gen);  // set random time
        lastUpdate = std::chrono::system_clock::now();
        
        // wait for cycle time
        while(true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            long timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate).count();

            if( timeSinceLastUpdate >=  cycleDuration )
            {
                break;
            }
        }
        // change traffic light color
        _currentPhase = ( _currentPhase == TrafficLightPhase::red ? TrafficLightPhase::green : TrafficLightPhase::red );
        
        // print a notification to standard output
        std::unique_lock<std::mutex> lck(_mutex);
        std::cout << "a traffic ligt toggeled from " << (_currentPhase == TrafficLightPhase::red ? "green " : "red ") << "to " << (_currentPhase == TrafficLightPhase::red ? "red " : "green ") << std::endl;
        lck.unlock();

        _messageQueue->send(std::move(getCurrentPhase()));

    }
}