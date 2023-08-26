/**
 * @file        power_measuring.cpp
 *
 * @author      Tobias Anker <tobias.anker@kitsunemimi.moe>
 *
 * @copyright   Apache License Version 2.0
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

#include <libKitsunemimiHanamiHardware/power_measuring.h>
#include <libKitsunemimiHanamiHardware/value_container.h>

#include <libKitsunemimiSakuraHardware/host.h>
#include <libKitsunemimiSakuraHardware/cpu_core.h>
#include <libKitsunemimiSakuraHardware/cpu_package.h>
#include <libKitsunemimiSakuraHardware/cpu_thread.h>

#include <libKitsunemimiJson/json_item.h>

PowerMeasuring* PowerMeasuring::instance = nullptr;

PowerMeasuring::PowerMeasuring()
    : Kitsunemimi::Thread("Azuki_EnergyMeasuring")
{
    m_valueContainer = new ValueContainer();
}

PowerMeasuring::~PowerMeasuring()
{
    delete m_valueContainer;
}

Kitsunemimi::DataMap*
PowerMeasuring::getJson()
{
    return m_valueContainer->toJson();
}

/**
 * @brief ThreadBinder::run
 */
void
PowerMeasuring::run()
{
    Kitsunemimi::ErrorContainer error;
    while(m_abort == false)
    {
        double power = 0.0;
        Kitsunemimi::Sakura::Host* host = Kitsunemimi::Sakura::Host::getInstance();
        for(uint64_t i = 0; i < host->cpuPackages.size(); i++) {
            power += host->getPackage(i)->getTotalPackagePower();
        }

        // HINT(kitsudaiki): first tests were made on a notebook. When this notebook came from
        // standby, then there was a single extremly high value, which broke the outout.
        // So this is only a workaround for this edge-case. I this there is no node,
        // which takes more then 1MW per node and so this workaround shouldn't break any setup.
        if(power > 1000000.0) {
            power = 0.0f;
        }

        m_valueContainer->addValue(power);

        sleep(1);
    }
}
