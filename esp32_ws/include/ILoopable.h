/**
 * @file ILoopable.h
 * @brief Interface struct for loopable devices
 * 
 * Defines a struct-based interface that devices can implement to provide
 * a standard loop behavior that can be called uniformly in the main loop.
 * 
 * @author Generated for Marble Track Project
 * @date 2025
 */

#ifndef ILOOPABLE_H
#define ILOOPABLE_H

/**
 * @struct ILoopable
 * @brief Interface struct for devices that need periodic loop execution
 * 
 * This struct defines the interface contract for devices that need to
 * perform periodic operations in the main loop. Uses pure virtual functions
 * to enforce the interface. In C++, structs can have virtual functions
 * just like classes.
 */
struct ILoopable {
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes
     */
    virtual ~ILoopable() = default;
    
    /**
     * @brief Loop function for continuous device operations
     * 
     * This function should be called repeatedly in the main loop to allow
     * the device to perform any periodic operations or state updates.
     */
    virtual void loop() = 0;
};

#endif // ILOOPABLE_H
