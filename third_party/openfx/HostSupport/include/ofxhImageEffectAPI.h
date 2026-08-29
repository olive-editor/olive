


#ifndef OFXH_IMAGE_EFFECT_API_H
#define OFXH_IMAGE_EFFECT_API_H



#include "ofxhPluginAPICache.h"
#include "ofxhPluginCache.h"
#include <string>
#include <map>
#include <set>
#include <memory>
#include <utility>

#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxhImageEffect.h"
#include "ofxhHost.h"

  
  
    
    namespace OFX::Host::ImageEffect {

      class PluginCache;

      /// subclass of Plugin representing an ImageEffect plugin.  used to store API-specific
      /// data
      class ImageEffectPlugin : public Plugin {

        PluginCache &_pc;

        // this comes off Descriptor's property set after a describe
        // context independent
        std::shared_ptr<Descriptor> _baseDescriptor; /// NEEDS TO BE MADE WITH A FACTORY FUNCTION ON THE HOST!!!!!!
        
        /// map to store contexts in
        std::map<std::string, std::shared_ptr<Descriptor>> _contexts;

        mutable std::set<std::string> _knownContexts;
        mutable bool _madeKnownContexts;

        std::unique_ptr<PluginHandle> _pluginHandle;

        void addContextInternal(const std::string &context) const;

      public:
			  ImageEffectPlugin(PluginCache &pc, PluginBinary *pb, int pi, OfxPlugin *pl);

        ImageEffectPlugin(PluginCache &pc,
                          PluginBinary *pb,
                          int pi,
                          const std::string &api,
                          int apiVersion,
                          const std::string &pluginId,
                          const std::string &rawId,
                          int pluginMajorVersion,
                          int pluginMinorVersion);

        ~ImageEffectPlugin() override;

        /// return the API handler this plugin was constructed by
        APICache::PluginAPICacheI &getApiHandler() override;


        /// get the base image effect descriptor
        Descriptor &getDescriptor();

        /// get the base image effect descriptor, const version
        const Descriptor &getDescriptor() const;

        /// get the image effect descriptor for the context
        Descriptor *getContext(const std::string &context);

        void addContext(const std::string &context);
        void addContext(const std::string &context, std::shared_ptr<Descriptor> ied);

        virtual void saveXML(std::ostream &os);

        const std::set<std::string>& getContexts() const;

        PluginHandle *getPluginHandle();

        void unload();

        /// this is called to make an instance of the effect
        /// the client data ptr is what is passed back to the client creation function
        ImageEffect::Instance* createInstance(const std::string &context, void *clientDataPtr);

      };

      class MajorPlugin {
        std::string _id;
        int _major;

      public:
        MajorPlugin(std::string id, int major) : _id(std::move(id)), _major(major) {
        }

        explicit MajorPlugin(ImageEffectPlugin *iep) : _id(iep->getIdentifier()), _major(iep->getVersionMajor()) {
        }

        [[nodiscard]] const std::string &getId() const {
          return _id;
        }

        [[nodiscard]] int getMajor() const {
          return _major;
        }

        bool operator<(const MajorPlugin &other) const {
          if (_id < other._id)
            return true;

          if (_id > other._id)
            return false;
          
          if (_major < other._major)
            return true;

          return false;
        }
      };

      /// implementation of the specific Image Effect handler API cache.
      class PluginCache : public APICache::PluginAPICacheI {
      public:      

      private:
        /// all plugins
        std::vector<ImageEffectPlugin *> _plugins;

        /// latest version of each plugin by ID
        std::map<std::string, ImageEffectPlugin *> _pluginsByID;

        /// latest minor version of each plugin by (ID,major)
        std::map<MajorPlugin, ImageEffectPlugin *> _pluginsByIDMajor;

        /// xml parsing state
        ImageEffectPlugin *_currentPlugin;
        /// xml parsing state
        Property::Property *_currentProp;
        
        Descriptor *_currentContext;
        Param::Descriptor *_currentParam;
        ClipDescriptor *_currentClip;

        /// pointer to our image effect host
        OFX::Host::ImageEffect::Host* _host;

      public:  

        explicit PluginCache(OFX::Host::ImageEffect::Host &host);

        ~PluginCache() override;        

        /// get the plugin by id.  vermaj and vermin can be specified.  if they are not it will
        /// pick the highest found version.
        ImageEffectPlugin *getPluginById(const std::string &id, int vermaj=-1, int vermin=-1);

        /// get the plugin by label.  vermaj and vermin can be specified.  if they are not it will
        /// pick the highest found version.
        ImageEffectPlugin *getPluginByLabel(const std::string &label, int vermaj=-1, int vermin=-1);

        OFX::Host::ImageEffect::Host *getHost() {
          return _host;
        }

        [[nodiscard]] const std::vector<ImageEffectPlugin *>& getPlugins() const;

        [[nodiscard]] const std::map<std::string, ImageEffectPlugin *>& getPluginsByID() const;

        [[nodiscard]] const std::map<MajorPlugin, ImageEffectPlugin *>& getPluginsByIDMajor() const
        {
          return _pluginsByIDMajor;
        }

        /// handle the case where the info needs filling in from the file.  runs the "describe" action on the plugin.
        void loadFromPlugin(Plugin *p) const override;
        
        /// handler for preparing to read in a chunk of XML from the cache, set up context to do this
        void beginXmlParsing(Plugin *p) override;

        /// XML handler : element begins (everything is stored in elements and attributes)       
        void xmlElementBegin(const std::string &el, std::map<std::string, std::string> map) override;

        void xmlCharacterHandler(const std::string &) override;
        
        void xmlElementEnd(const std::string &el) override;
        
        void endXmlParsing() override;
        
        void saveXML(Plugin *ip, std::ostream &os) const override;

        void confirmPlugin(Plugin *p) override;

        bool pluginSupported(Plugin *p, std::string &reason) const override;

        Plugin *newPlugin(PluginBinary *pb,
                          int pi,
                          OfxPlugin *pl) override;

        Plugin *newPlugin(PluginBinary *pb,
                          int pi,
                          const std::string &api,
                          int apiVersion,
                          const std::string &pluginId,
                          const std::string &rawId,
                          int pluginMajorVersion,
                          int pluginMinorVersion) override;

        void dumpToStdOut();
      };
    
    } // ImageEffect

  // Host

// OFX

#endif
