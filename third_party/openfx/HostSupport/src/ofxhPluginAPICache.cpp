

#include <assert.h>

#include <string>
#include <map>

// ofx
#include "ofxCore.h"
#include "ofxImageEffect.h"

// ofx host
#include "ofxhBinary.h"
#include "ofxhPropertySuite.h"
#include "ofxhClip.h"
#include "ofxhParam.h"
#include "ofxhMemory.h"
#include "ofxhImageEffect.h"
#include "ofxhPluginAPICache.h"
#include "ofxhPluginCache.h"
#include "ofxhHost.h"
#include "ofxhImageEffectAPI.h"
#include "ofxhXml.h"

namespace OFX
{
  namespace Host
  {
    namespace APICache
    {
      void PluginAPICacheI::registerInCache(OFX::Host::PluginCache &pluginCache) {
        pluginCache.registerAPICache(_apiName, _apiVersionMin, _apiVersionMax, this);
      }      

      void propertySetXMLRead(const std::string &el,
                              std::map<std::string, std::string> map,
                              Property::Set &set,
                              Property::Property *&currentProp) {
        if (el == "property") {
          std::string propName = map["name"];
          std::string propType = map["type"];
          int dimension = atoi(map["dimension"].c_str());
          
          currentProp = set.fetchProperty(propName, false);
          
          if(!currentProp) {
            if (propType == "int") {
              currentProp = new Property::Int(propName, dimension, false, 0);
            } else if (propType == "string") {
              currentProp = new Property::String(propName, dimension, false, "");
            } else if (propType == "double") {
              currentProp = new Property::Double(propName, dimension, false, 0);
            } else if (propType == "pointer") {
              currentProp = new Property::Pointer(propName, dimension, false, 0);
            }
            set.addProperty(currentProp);
          }
          return;
        }
        
        if (el == "value" && currentProp) {
          int index = atoi(map["index"].c_str());
          std::string value = map["value"];
          
          switch (currentProp->getType()) {
          case Property::eInt:
            set.setIntProperty(currentProp->getName(), atoi(value.c_str()), index);
            break;
          case Property::eString:
            set.setStringProperty(currentProp->getName(), value, index);
            break;
          case Property::eDouble:
            set.setDoubleProperty(currentProp->getName(), atof(value.c_str()), index);
            break;
          case Property::ePointer:
            break;
          default:
            break;
          }

          return;
        }

        std::cout << "got unrecognised key " << el << "\n";

        assert(false);
      }
      
      static void propertyXMLWrite(std::ostream &o, Property::Property *prop, const std::string &indent="")
      {        
        if (prop->getType() != Property::ePointer)  {
          
          o << indent << "<property "
            << XML::attribute("name", prop->getName())
            << XML::attribute("type", Property::gTypeNames[prop->getType()])
            << XML::attribute("dimension", prop->getFixedDimension()) 
            << ">\n";
          
          for (int i=0;i<prop->getDimension();i++) {
            o << indent << "  <value " 
              << XML::attribute("index", i)
              << XML::attribute("value", prop->getStringValue(i)) 
              << "/>\n";
          }
          
          o << indent << "</property>\n";
        }
      }

      void propertyXMLWrite(std::ostream &o, const Property::Set &set, const std::string &name, int indent)
      {
        Property::Property *prop = set.fetchProperty(name);

        if(prop) {
          std::string indent_prefix(indent, ' ');
          propertyXMLWrite(o, prop, indent_prefix);
        }
      }

      void propertySetXMLWrite(std::ostream &o, const Property::Set &set, int indent) 
      {
        std::string indent_prefix(indent, ' ');

        for (Property::PropertyMap::const_iterator i = set.getProperties().begin();
             i != set.getProperties().end();
             i++)
          {
            Property::Property *prop = i->second;
            propertyXMLWrite(o, prop, indent_prefix);
          }
      }

    }
  }
}
