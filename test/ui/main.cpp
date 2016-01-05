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

#include "mbed-drivers/mbed.h"

#include "uif-ui-popup-alert-wrd/AlertView.h"

#include "core-util/SharedPointer.h"
#include "cborg/cbor.h"
#include "mbed-block/BlockDynamic.h"
#include "message-center/MessageCenter.h"

#include "uiframework/UIFramework.h"
#include "uif-matrixlcd/MatrixLCD.h"

static SPI lcdspi(LCD_SPI_MOSI, NC, LCD_SPI_CLK);
static uif::MatrixLCD lcd(lcdspi, LCD_CS, LCD_DISP, LCD_EXT_COM_IN);
static SharedPointer<UIFramework> uiFramework;

/*****************************************************************************/

static SharedPointer<BlockStatic> sendBlock;

static const int OVERHEAD = 1 + 4 + 4 + 4;
static const int ITEMS = 3;
static const int TIMEONSCREEN = 5000;
static const int DELAY = 1000;

static const char title[] = "Title - the title";
static const char body[] = "Body - the body can be really long";

void sendTaskDone()
{
    // free block
    sendBlock = SharedPointer<BlockStatic>();
}

void popup()
{
    // allocate memory for the message.
    sendBlock = SharedPointer<BlockStatic>(new BlockDynamic(OVERHEAD + sizeof(title) + sizeof(body)));

    // cbor encode message.
    Cbore encoder(sendBlock->getData(), sendBlock->getLength());

    encoder.array(ITEMS)
           .item(TIMEONSCREEN)
           .item(title)
           .item(body);

    // set length of message.
    sendBlock->setLength(encoder.getLength());

    // use message center to send messages to the popup alert view.
    MessageCenter::sendTask(MessageCenter::LocalHost,
                            MessageCenter::AlertPort,
                            *sendBlock.get(),
                            sendTaskDone);

    // resend message after a short delay.
    minar::Scheduler::postCallback(popup)
        .delay(minar::milliseconds(TIMEONSCREEN + DELAY));
}

void app_start(int, char *[])
{
    SharedPointer<UIView> view(new AlertView());
    view->setWidth(128);
    view->setHeight(128);

    // UI framework
    uiFramework = SharedPointer<UIFramework>(new UIFramework(lcd, view));

    minar::Scheduler::postCallback(popup)
        .delay(minar::milliseconds(DELAY));
}
