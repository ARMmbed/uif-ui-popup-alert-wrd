/*
 * Copyright (c) 2016, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __UIF_UI_ALERT_VIEW_H__
#define __UIF_UI_ALERT_VIEW_H__

#include "UIFramework/UIView.h"
#include "UIFramework/UITextView.h"

#include "mbed-block/BlockStatic.h"

class AlertView : public UIView
{
public:
    /**
     * @brief Constructor for the AlertView UIView-class.
     * @details The AlertView object listens to the MessageCenter AlertPort for
     *          CBOR encoded messages. The expected format is an array with 3
     *          items, the first an unsigned integer with the on-screen time in
     *          milliseconds. The second, a string with the title. The third, a
     *          string with the body of the message.
     */
    AlertView(void);

    /**
     * @brief Destructor for the AlertView UIView-class.
     */
    ~AlertView(void);

    /**
     * @brief Function for handling messages received through Message Center.
     * @details This function is responsible for decoding the CBOR message and
     *          setting up the UIView objects.
     *
     * @param block BlockStatic object with the CBOR encoded message.
     */
    void receivedAlert(BlockStatic block);

    /**
     * @brief Function for clearing the popup alert.
     * @details This function cleans up the UIView objects and wakes up the
     *          UIFramework.
     */
    void clearAlert(void);

    /**
     * @brief Overlay the frame buffer with the popup alert if active.
     *
     * @param canvas FrameBuffer-object wrapped in a SharedPointer.
     * @param xOffset Number of pixels the camera has been translated along the
     *        horizontal axis.
     * @param yOffset Number of pixels the camera has been translated along the
     *        vertical axis.
     * @return The time in milliseconds to when the object wants to be called
     *         again. This is a lower-bound, meaning calling the function sooner
     *         will only result in the same data being filled into the frame
     *         buffer again.
     */
    virtual uint32_t fillFrameBuffer(SharedPointer<FrameBuffer>& buffer, int16_t xOffset, int16_t yOffset);

private:
    bool showTitle;
    bool showLine[6];

    std::string titleString;
    std::string messageString;

    SharedPointer<UITextView> titleCell;
    SharedPointer<UITextView> lineCell[6];

    minar::callback_handle_t clearAlertHandle;
};

#endif // __UIF_UI_ALERT_VIEW_H__
