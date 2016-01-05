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

#include "uif-ui-popup-alert-wrd/AlertView.h"

#include "cborg/Cbor.h"
#include "core-util/FunctionPointer.h"
#include "message-center/MessageCenter.h"

using namespace mbed::util;

#if 0
#include "swo/swo.h"
#define DEBUGOUT(...) { swoprintf(__VA_ARGS__); }
#else
#define DEBUGOUT(...)
#endif

#define LINE_WIDTH 17

/**
 * @brief Is the character printable?
 *
 * @param character Character to be checked.
 * @return Answer to the question above.
 */
static bool printable(char character)
{
    return ((' ' <= character) && (character <= '~'));
}

/**
 * @brief Convert upper case character to lower case.
 * @details Lower case characters are unchanged.
 *
 * @param character Character to be converted.
 * @return Lower case character.
 */
static char lowerCase(char character)
{
    if (('A' <= character) && (character <= 'Z'))
    {
        return character - 'A' + 'a';
    }
    else
    {
        return character;
    }
}

/**
 * @brief Find the next character after start index that is not space.
 *
 * @param line String to be searched.
 * @param start Start index to search from.
 *
 * @return Index to next non-space character.
 */
static std::size_t whitespace(std::string& line, std::size_t start)
{
    for (; start < line.length(); start++)
    {
        if (line[start] != ' ')
        {
            return start;
        }
    }

    return start;
}

AlertView::AlertView()
    :   UIView(),
        showTitle(false),
        clearAlertHandle(NULL)
{
    showLine[0] = false;
    showLine[1] = false;
    showLine[2] = false;
    showLine[3] = false;
    showLine[4] = false;
    showLine[5] = false;

    /* Setup listener function in the Message Center. */
    FunctionPointer1<void, BlockStatic> fp(this, &AlertView::receivedAlert);

    MessageCenter::addListenerTask(MessageCenter::LocalHost,
                                   MessageCenter::AlertPort,
                                   fp);
}

AlertView::~AlertView()
{

}

void AlertView::receivedAlert(BlockStatic block)
{
    for (size_t idx = 0; idx < block.getLength(); idx++)
    {
        DEBUGOUT("%02X", block.at(idx));
    }
    DEBUGOUT("\r\n");

    uint32_t millisecondsOnScreen;
    std::string valueString;

    // decode message
    Cborg cbor(block.getData(), block.getLength());

    // read onscreen time
    cbor.at(0).getUnsigned(&millisecondsOnScreen);

    // read title
    cbor.at(1).getString(valueString);

    // remove non-printable characters, extra white space, and convert to lower case.
    titleString = "";
    for (std::size_t idx = 0; idx < valueString.length(); idx++)
    {
        char character = valueString[idx];

        if (printable(character))
        {
            titleString.push_back(lowerCase(character));
        }
    }

    // create UIView object and set display flag
    showTitle = true;
    titleCell = SharedPointer<UITextView>(new UITextView(titleString, &Font_Breadcrumbs));

    // read body
    cbor.at(2).getString(valueString);

    // remove non-printable characters, extra white space, and convert to lower case.
    messageString = "";
    for (std::size_t idx = 0; idx < valueString.length(); idx++)
    {
        char character = valueString[idx];

        if (printable(character))
        {
            messageString.push_back(lowerCase(character));
        }
    }

    // Create UIView objects.
    // Body text longer than LINE_WIDTH is split up in multiple lines.
    std::string line = "";
    std::size_t length = messageString.length();
    std::size_t index = 0;

    for (std::size_t cellIndex = 0; cellIndex < 6; cellIndex++)
    {
        index = whitespace(messageString, index);

        if (index < length)
        {
            line = messageString.substr(index, LINE_WIDTH);
            lineCell[cellIndex] = SharedPointer<UITextView>(new UITextView(line, &Font_Breadcrumbs));
            showLine[cellIndex] = true;
            index += line.length();
        }
        else
        {
            break;
        }
    }

    /* If message was succesfully decoded, wake up UIFramework, and set callback call
       to clear alert after the given time period.
    */
    if (showTitle || showLine[0])
    {
        minar::Scheduler::postCallback(wakeupCallback);

        clearAlertHandle = minar::Scheduler::postCallback(this, &AlertView::clearAlert)
                                .delay(minar::milliseconds(millisecondsOnScreen))
                                .getHandle();
    }
}

void AlertView::clearAlert()
{
    // cancel callback if present
    if (clearAlertHandle)
    {
        clearAlertHandle = NULL;
        minar::Scheduler::cancelCallback(clearAlertHandle);
    }

    // clean up
    showTitle = false;
    showLine[0] = false;
    showLine[1] = false;
    showLine[2] = false;
    showLine[3] = false;
    showLine[4] = false;
    showLine[5] = false;

    // wake up UIFramework
    minar::Scheduler::postCallback(wakeupCallback);
}

uint32_t AlertView::fillFrameBuffer(SharedPointer<FrameBuffer>& canvas, int16_t xOffset, int16_t yOffset)
{
    /*
    +-------------------------------------------------------------------------+
    |                                                                         |
    |  20                                                                     |
    |     +---------------------------------------------------------+         |
    |     |                                                         |  5 1    |
    |     |                                                         | +-++    |
    |     |                                                         | | ||    |
    |     |                                                         | | ||    |
    |     |                        110 x 85                         | | ||    |
    |  6  |                                                         | | ||    |
    |     |                                                         | | ||    |
    |     |                                                         | | ||    |
    |     |                                                         | | ||    |
    |     |                                                         | | ||    |
    |     +---------------------------------------------------------+ | ||    |
    |                                                                 | ||    |
    |        +--------------------------------------------------------+ ||    |
    |      5 |                                                          ||    |
    |        +----------------------------------------------------------+|    |
    |      1 +-----------------------------------------------------------+    |
    |                                                                         |
    +-------------------------------------------------------------------------+
    */

    if (showTitle || showLine[0])
    {
        /* big canvas, white background */
        SharedPointer<FrameBuffer> background = canvas->getFrameBuffer(6, 20,
                                                                       110, 85);

        background->drawRectangle(0, background->getWidth(),
                                  0, background->getHeight(), 1);

        /* black border to the right */
        background = canvas->getFrameBuffer(116, 25,
                                            5, 80);

        background->drawRectangle(0, background->getWidth(),
                                  0, background->getHeight(), 0);

        /* white edge to the right */
        background = canvas->getFrameBuffer(121, 30,
                                            1, 80);

        background->drawRectangle(0, background->getWidth(),
                                  0, background->getHeight(), 1);

        /* black border at the bottom */
        background = canvas->getFrameBuffer(6 + 5, 20 + 85,
                                            110, 5);

        background->drawRectangle(0, background->getWidth(),
                                  0, background->getHeight(), 0);

        /* white edge at the bottom */
        background = canvas->getFrameBuffer(6 + 5, 20 + 85 + 5,
                                            111, 1);

        background->drawRectangle(0, background->getWidth(),
                                  0, background->getHeight(), 1);

        /*************************************************************************/

        /* title */
        if (showTitle)
        {
            SharedPointer<FrameBuffer> titleCanvas =
                                    canvas->getFrameBuffer(6 + 6, 20 + 6,
                                                           110 - 12, 20);

            titleCell->fillFrameBuffer(titleCanvas, xOffset, yOffset);
        }

        /* message */
        for (std::size_t cellIndex = 0; cellIndex < 6; cellIndex++)
        {
            if (showLine[cellIndex])
            {
                SharedPointer<FrameBuffer> lineCanvas =
                                    canvas->getFrameBuffer(12, 44 + 10 * cellIndex,
                                                           98, 20);

                lineCell[cellIndex]->fillFrameBuffer(lineCanvas, xOffset, yOffset);
            }
            else
            {
                break;
            }
        }
    }

    return ULONG_MAX;
}
