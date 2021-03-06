// Copyright (C) 2011 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <mutex>

#include <simgear/compiler.h>

#include "RTIFederateFactoryRegistry.hxx"

#include "simgear/threads/std::lock_guard.hxx"
#include "RTIFederate.hxx"

namespace simgear {

RTIFederateFactoryRegistry::RTIFederateFactoryRegistry()
{
}

RTIFederateFactoryRegistry::~RTIFederateFactoryRegistry()
{
}

SGSharedPtr<RTIFederate>
RTIFederateFactoryRegistry::create(const std::string& name, const std::list<std::string>& stringList) const
{
    std::lock_guard<std::mutex> guard(_mutex);
    for (FederateFactoryList::const_iterator i = _federateFactoryList.begin(); i != _federateFactoryList.end(); ++i) {
        SGSharedPtr<RTIFederate> federate = (*i)->create(name, stringList);
        if (!federate.valid())
            continue;
        return federate;
    }
    return NULL;
}

void
RTIFederateFactoryRegistry::registerFactory(RTIFederateFactory* factory)
{
    std::lock_guard<std::mutex> guard(_mutex);
    _federateFactoryList.push_back(factory);
}

const SGSharedPtr<RTIFederateFactoryRegistry>&
RTIFederateFactoryRegistry::instance()
{
    static SGSharedPtr<RTIFederateFactoryRegistry> registry = new RTIFederateFactoryRegistry;
    return registry;
}

}
