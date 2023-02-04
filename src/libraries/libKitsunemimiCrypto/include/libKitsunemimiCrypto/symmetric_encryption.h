/**
 *  @file       symatic_encryption.h
 *
 *  @author     Tobias Anker <tobias.anker@kitsunemimi.moe>
 *
 *  @copyright  Apache License Version 2.0
 *
 *      Copyright 2022 Tobias Anker
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#ifndef SYMMETRIC_ENCRYPTION_H
#define SYMMETRIC_ENCRYPTION_H

#include <iostream>
#include <libKitsunemimiCommon/buffer/data_buffer.h>
#include <libKitsunemimiCommon/logger.h>

#include <cryptopp/sha.h>
#include <cryptopp/aes.h>
#include <cryptopp/files.h>
#include <cryptopp/modes.h>

namespace Kitsunemimi
{

//--------------------------------------------------------------------------------------------------
// AES256
//--------------------------------------------------------------------------------------------------
bool encrypt_AES_256_CBC(std::string &result,
                         const std::string &input,
                         const CryptoPP::SecByteBlock &key,
                         Kitsunemimi::ErrorContainer &error);

bool decrypt_AES_256_CBC(std::string &result,
                         const std::string &input,
                         const CryptoPP::SecByteBlock &key,
                         Kitsunemimi::ErrorContainer &error);

} // namespace Kitsunemimi

#endif // SYMMETRIC_ENCRYPTION_H
