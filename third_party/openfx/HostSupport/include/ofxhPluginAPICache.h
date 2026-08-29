

#ifndef OFX_PLUGIN_API_CACHE
#define OFX_PLUGIN_API_CACHE

// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause

#include <string>
#include <iostream>
#include <ostream>
#include <map>

#include "ofxhPropertySuite.h"

namespace OFX
{
  namespace Host {
    class Plugin;
    class PluginBinary;
    class PluginCache;

    namespace ImageEffect {
      class ImageEffectDescriptor;
    }
  }
}

namespace OFX
{
  namespace Host
  {
    namespace APICache {

      /// this acts as an interface for the Plugin Cache, handling api-specific cacheing
      class PluginAPICacheI
      {
      protected:
        std::string _apiName;
        int _apiVersionMin, _apiVersionMax;
      public:
        PluginAPICacheI(const std::string &apiName, int verMin, int verMax)
          : _apiName(apiName)
            , _apiVersionMin(verMin)
            , _apiVersionMax(verMax)
        {
        }
        
        virtual ~PluginAPICacheI() {}
        
        virtual void loadFromPlugin(Plugin *) const = 0;

        /// factory method, to create a new plugin (from binary)
        virtual Plugin *newPlugin(PluginBinary *, int pi, OfxPlugin *plug) = 0;

        /// factory method, to create a new plugin (from the 
        virtual Plugin *newPlugin(PluginBinary *pb, int pi, const std::string &api, int apiVersion, const std::string &pluginId,
                                  const std::string &rawId, int pluginMajorVersion, int pluginMinorVersion) = 0;

        virtual void beginXmlParsing(Plugin *) = 0;
        virtual void xmlElementBegin(const std::string &, std::map<std::string, std::string>) = 0;
        virtual void xmlCharacterHandler(const std::string &) = 0;
        virtual void xmlElementEnd(const std::string &) = 0;
        virtual void endXmlParsing() = 0;
        
        virtual void saveXML(Plugin *, std::ostream &) const = 0;

        virtual void confirmPlugin(Plugin *) = 0;

        virtual bool pluginSupported(Plugin *, std::string &reason) const = 0;

        void registerInCache(OFX::Host::PluginCache &pluginCache);
      };
      
      /// helper function to build a property set from XML. Really should be a member of the property set!!!
      void propertySetXMLRead(const std::string &el, std::map<std::string, std::string> map, Property::Set &set, Property::Property*&);

      /// helper function to write a property set to XML. Really should be a member of the property set!!!
      void propertySetXMLWrite(std::ostream &o, const Property::Set &set, int indent=0);

      /// helper function to write a single property from a set to XML. Really should be a member of the property set!!!
      void propertyXMLWrite(std::ostream &o, const Property::Set &set, const std::string &name, int indent=0);

    }
  }
}
#endif
