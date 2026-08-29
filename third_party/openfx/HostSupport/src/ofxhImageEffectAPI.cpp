

#include <assert.h>

#include <memory>
#include <string>
#include <map>
#include <ctype.h>

// ofx
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

// Disable the "this pointer used in base member initialiser list" warning in Windows
namespace OFX {

  namespace Host {

    namespace ImageEffect {

      /// our global host bobject, set when the cache is created
      OFX::Host::ImageEffect::Host *gImageEffectHost;

      /// ctor
#ifdef WINDOWS
#pragma warning( disable : 4355 )
#endif
      ImageEffectPlugin::ImageEffectPlugin(PluginCache &pc, PluginBinary *pb, int pi, OfxPlugin *pl)
        : Plugin(pb, pi, pl)
        , _pc(pc)
        , _baseDescriptor(NULL)
        , _madeKnownContexts(false)
      {
        _baseDescriptor = gImageEffectHost->makeDescriptor(this);
      }

      ImageEffectPlugin::ImageEffectPlugin(PluginCache &pc,
                                           PluginBinary *pb,
                                           int pi,
                                           const std::string &api,
                                           int apiVersion,
                                           const std::string &pluginId,
                                           const std::string &rawId,
                                           int pluginMajorVersion,
                                           int pluginMinorVersion)
        : Plugin(pb, pi, api, apiVersion, pluginId, rawId, pluginMajorVersion, pluginMinorVersion)
        , _pc(pc)
        , _baseDescriptor(NULL) 
        , _madeKnownContexts(false)
      {        
        _baseDescriptor = gImageEffectHost->makeDescriptor(this);
      }

#ifdef WINDOWS
#pragma warning( default : 4355 )
#endif

      ImageEffectPlugin::~ImageEffectPlugin()
      {
        _contexts.clear();
        if(_pluginHandle) {
          OfxPlugin *op = _pluginHandle->getOfxPlugin();
          OfxStatus stat;
          try {
#           ifdef OFX_DEBUG_ACTIONS
              std::cout << "OFX: "<<(void*)op<<"->"<<kOfxActionUnload<<"()"<<std::endl;
#           endif
            stat = op->mainEntry(kOfxActionUnload, 0, 0, 0);
#           ifdef OFX_DEBUG_ACTIONS
              std::cout << "OFX: "<<(void*)op<<"->"<<kOfxActionUnload<<"()->"<<StatStr(stat)<<std::endl;
#           endif
          } CatchAllSetStatus(stat, gImageEffectHost, op, kOfxActionUnload);
          (void)stat;
        }
      }

      APICache::PluginAPICacheI &ImageEffectPlugin::getApiHandler()
      {
        return _pc;
      }


      /// get the image effect descriptor
      Descriptor &ImageEffectPlugin::getDescriptor() {
        return *_baseDescriptor;
      }

      /// get the image effect descriptor const version
      const Descriptor &ImageEffectPlugin::getDescriptor() const {
        return *_baseDescriptor;
      }

      void ImageEffectPlugin::addContext(const std::string &context, std::shared_ptr<Descriptor> ied)
      {
        _contexts[context] = ied;
        _knownContexts.insert(context);
        _madeKnownContexts = true;
      }

      void ImageEffectPlugin::addContext(const std::string &context)
      {
        _knownContexts.insert(context);
        _madeKnownContexts = true;
      }

      void ImageEffectPlugin::addContextInternal(const std::string &context) const
      {
        _knownContexts.insert(context);
        _madeKnownContexts = true;
      }

      void ImageEffectPlugin::saveXML(std::ostream &os) 
      {        
        APICache::propertySetXMLWrite(os, getDescriptor().getProps(), 6);
      }

      const std::set<std::string> &ImageEffectPlugin::getContexts() const {
        if (_madeKnownContexts) {
          return _knownContexts;
        } 
        else {
          const OFX::Host::Property::Set &eProps = getDescriptor().getProps();
          int size = eProps.getDimension(kOfxImageEffectPropSupportedContexts);
          for (int j=0;j<size;j++) {
            std::string context = eProps.getStringProperty(kOfxImageEffectPropSupportedContexts, j);
            addContextInternal(context);
          }
          return _knownContexts;
        }
      }

      PluginHandle *ImageEffectPlugin::getPluginHandle() 
      {
        if(!_pluginHandle) {
          _pluginHandle.reset(new OFX::Host::PluginHandle(this, _pc.getHost())); 
          
          OfxPlugin *op = _pluginHandle->getOfxPlugin();
          
          if (!op) {
            _pluginHandle.reset();
            return nullptr;
          }

          OfxStatus stat;
          try {
#           ifdef OFX_DEBUG_ACTIONS
              std::cout << "OFX: "<<(void*)op<<"->"<<kOfxActionLoad<<"()"<<std::endl;
#           endif
            stat = op->mainEntry(kOfxActionLoad, 0, 0, 0);
#           ifdef OFX_DEBUG_ACTIONS
              std::cout << "OFX: "<<(void*)op<<"->"<<kOfxActionLoad<<"()->"<<StatStr(stat)<<std::endl;
#           endif
          } CatchAllSetStatus(stat, gImageEffectHost, op, kOfxActionLoad);

          if (stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
            _pluginHandle.reset();
            return nullptr;
          }
          
          try {
#           ifdef OFX_DEBUG_ACTIONS
              std::cout << "OFX: "<<(void*)op<<"->"<<kOfxActionDescribe<<"()"<<std::endl;
#           endif
            stat = op->mainEntry(kOfxActionDescribe, getDescriptor().getHandle(), 0, 0);
#           ifdef OFX_DEBUG_ACTIONS
              std::cout << "OFX: "<<(void*)op<<"->"<<kOfxActionDescribe<<"()->"<<StatStr(stat)<<std::endl;
#           endif
          } CatchAllSetStatus(stat, gImageEffectHost, op, kOfxActionDescribe);

          if (stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
            _pluginHandle.reset();
            return nullptr;
          }
        }

        return _pluginHandle.get();
      }

      Descriptor *ImageEffectPlugin::getContext(const std::string &context) 
      {
        std::map<std::string, std::shared_ptr<Descriptor>>::iterator it = _contexts.find(context);

        if (it != _contexts.end()) {
          //printf("found context description.\n");
          return it->second.get();
        }

        if (_knownContexts.find(context) == _knownContexts.end()) {
          return nullptr;
        }

        //        printf("doing context description.\n");

        OFX::Host::Property::PropSpec inargspec[] = {
          { kOfxImageEffectPropContext, OFX::Host::Property::eString, 1, true, context.c_str() },
            Property::propSpecEnd
        };
        
        OFX::Host::Property::Set inarg(inargspec);

        PluginHandle *ph = getPluginHandle();
        if (!ph) {
          return nullptr;
        }
        std::shared_ptr<ImageEffect::Descriptor> newContext( gImageEffectHost->makeDescriptor(getDescriptor(), this));

        OfxStatus stat;
        try {
#         ifdef OFX_DEBUG_ACTIONS
            std::cout << "OFX: "<<(void*)ph->getOfxPlugin()<<"->"<<kOfxImageEffectActionDescribeInContext<<"("<<context<<")"<<std::endl;
#         endif
          stat = ph->getOfxPlugin()->mainEntry(kOfxImageEffectActionDescribeInContext, newContext->getHandle(), inarg.getHandle(), 0);
#         ifdef OFX_DEBUG_ACTIONS
            std::cout << "OFX: "<<(void*)ph->getOfxPlugin()<<"->"<<kOfxImageEffectActionDescribeInContext<<"("<<context<<")->"<<StatStr(stat)<<std::endl;
#         endif
        } CatchAllSetStatus(stat, gImageEffectHost, ph->getOfxPlugin(), kOfxImageEffectActionDescribeInContext);

        if (stat == kOfxStatOK || stat == kOfxStatReplyDefault) {
          _contexts[context] = std::move(newContext);
          return _contexts[context].get();
        }
        return nullptr;
      }

      ImageEffect::Instance* ImageEffectPlugin::createInstance(const std::string &context, void *clientData)
      {          

        /// todo - we need to make sure action:load is called, then action:describe again
        /// (not because we are expecting the results to change, but because plugin
        /// might get confused otherwise), then a describe_in_context

        PluginHandle *ph = getPluginHandle();
        if (!ph) {
          return nullptr;
        }

        Descriptor *desc = getContext(context);
        
        if (desc) {
          ImageEffect::Instance *instance = gImageEffectHost->newInstance(clientData,
                                                                          this,
                                                                          *desc,
                                                                          context);
          instance->populate();
          return instance;
        }
        return 0;
      }

      void ImageEffectPlugin::unload() {
        if (_pluginHandle) {
          OfxStatus stat;
          try {
#           ifdef OFX_DEBUG_ACTIONS
              std::cout << "OFX: "<<(void*)_pluginHandle->getOfxPlugin()<<"->"<<kOfxActionUnload<<"()"<<std::endl;
#           endif
            stat = (*_pluginHandle)->mainEntry(kOfxActionUnload, 0, 0, 0);
#           ifdef OFX_DEBUG_ACTIONS
              std::cout << "OFX: "<<(void*)_pluginHandle->getOfxPlugin()<<"->"<<kOfxActionUnload<<"()->"<<StatStr(stat)<<std::endl;
#           endif
          } CatchAllSetStatus(stat, gImageEffectHost, (*_pluginHandle), kOfxActionUnload);
          (void)stat;
        }
      }

      PluginCache::PluginCache(OFX::Host::ImageEffect::Host &host) 
        : PluginAPICacheI(kOfxImageEffectPluginApi, 1, 1)
        , _currentPlugin(0)
        , _currentProp(0)
        , _currentContext(0)
        , _currentParam(0)
        , _currentClip(0)
        , _host(&host)
      {
        gImageEffectHost = &host;
      }

      PluginCache::~PluginCache() {}       

      /// get the plugin by id.  vermaj and vermin can be specified.  if they are not it will
      /// pick the highest found version.
      ImageEffectPlugin *PluginCache::getPluginById(const std::string &id, int vermaj, int vermin)
      {
        // return the highest version one, which fits the pattern provided
        ImageEffectPlugin *sofar = 0;
        std::string identifier = id;

        // Who says the pluginIdentifier is case-insensitive? OFX 1.3 spec doesn't mention this.
        // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#id472588
        //for (size_t i=0;i<identifier.size();i++) {
        //    identifier[i] = tolower(identifier[i]);
        //}

        for (std::vector<ImageEffectPlugin *>::iterator i=_plugins.begin();i!=_plugins.end();i++) {
          ImageEffectPlugin *p = *i;

          if (p->getIdentifier() != identifier) {
            continue;
          }

          if (vermaj != -1 && p->getVersionMajor() != vermaj) {
            continue;
          }

          if (vermin != -1 && p->getVersionMinor() != vermin) {
            continue;
          }

          if (!sofar || p->trumps(sofar)) {
            sofar = p;
          }
        }

        return sofar;
      }

      /// whether we support this plugin.  
      bool PluginCache::pluginSupported(OFX::Host::Plugin *p, std::string &reason) const {
        if (!gImageEffectHost) {
          reason = "host not initialized";
          return false;
        }
        auto *plugin = dynamic_cast<OFX::Host::ImageEffect::ImageEffectPlugin *>(p);
        if (!plugin) {
          reason = "not an image effect plugin";
          return false;
        }
        return gImageEffectHost->pluginSupported(plugin, reason);
      }

      /// get the plugin by label.  vermaj and vermin can be specified.  if they are not it will
      /// pick the highest found version.
      ImageEffectPlugin *PluginCache::getPluginByLabel(const std::string &label, int vermaj, int vermin)
      {
        // return the highest version one, which fits the pattern provided
        ImageEffectPlugin *sofar = 0;

        for (std::vector<ImageEffectPlugin *>::iterator i=_plugins.begin();i!=_plugins.end();i++) {
          ImageEffectPlugin *p = *i;

          if (p->getDescriptor().getProps().getStringProperty(kOfxPropLabel) != label) {
            continue;
          }

          if (vermaj != -1 && p->getVersionMajor() != vermaj) {
            continue;
          }

          if (vermin != -1 && p->getVersionMinor() != vermin) {
            continue;
          }

          if (!sofar || p->trumps(sofar)) {
            sofar = p;
          }
        }

        return sofar;
      }

      const std::vector<ImageEffectPlugin *>& PluginCache::getPlugins() const
      {
        return _plugins;
      }

      const std::map<std::string, ImageEffectPlugin *>& PluginCache::getPluginsByID() const
      {
        return _pluginsByID;
      }

      /// handle the case where the info needs filling in from the file.  runs the "describe" action on the plugin.
      void PluginCache::loadFromPlugin(Plugin *op) const {
        std::string msg = "loading ";
        msg += op->getRawIdentifier();

        _host->loadingStatus(msg);

        ImageEffectPlugin *p = dynamic_cast<ImageEffectPlugin*>(op);
        assert(p);

        PluginHandle plug(p, _host);

        OfxStatus stat;
        try {
#         ifdef OFX_DEBUG_ACTIONS
            std::cout << "OFX: "<<(void*)plug.getOfxPlugin()<<"->"<<kOfxActionLoad<<"()"<<std::endl;
#         endif
          stat = plug->mainEntry(kOfxActionLoad, 0, 0, 0);
#         ifdef OFX_DEBUG_ACTIONS
            std::cout << "OFX: "<<(void*)plug.getOfxPlugin()<<"->"<<kOfxActionLoad<<"()->"<<StatStr(stat)<<std::endl;
#         endif
        } CatchAllSetStatus(stat, gImageEffectHost, plug, kOfxActionLoad);

        if (stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
          std::cerr << "load failed on plugin " << op->getIdentifier() << std::endl;          
          return;
        }

        try {
#         ifdef OFX_DEBUG_ACTIONS
            std::cout << "OFX: "<<(void*)plug.getOfxPlugin()<<"->"<<kOfxActionDescribe<<"()"<<std::endl;
#         endif
          stat = plug->mainEntry(kOfxActionDescribe, p->getDescriptor().getHandle(), 0, 0);
#         ifdef OFX_DEBUG_ACTIONS
            std::cout << "OFX: "<<(void*)plug.getOfxPlugin()<<"->"<<kOfxActionDescribe<<"()->"<<StatStr(stat)<<std::endl;
#         endif
        } CatchAllSetStatus(stat, gImageEffectHost, plug, kOfxActionDescribe);

        if (stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
          std::cerr << "describe failed on plugin " << op->getIdentifier() << std::endl;          
          return;
        }

        ImageEffect::Descriptor &e = p->getDescriptor();
        Property::Set &eProps = e.getProps();

        int size = eProps.getDimension(kOfxImageEffectPropSupportedContexts);

        for (int j=0;j<size;j++) {
          std::string context = eProps.getStringProperty(kOfxImageEffectPropSupportedContexts, j);
          p->addContext(context);
        }

        try {
#         ifdef OFX_DEBUG_ACTIONS
            std::cout << "OFX: "<<(void*)plug.getOfxPlugin()<<"->"<<kOfxActionUnload<<"()"<<std::endl;
#         endif
          stat = plug->mainEntry(kOfxActionUnload, 0, 0, 0);
#         ifdef OFX_DEBUG_ACTIONS
            std::cout << "OFX: "<<(void*)plug.getOfxPlugin()<<"->"<<kOfxActionUnload<<"()->"<<StatStr(stat)<<std::endl;
#         endif
        } CatchAllSetStatus(stat, gImageEffectHost, plug, kOfxActionUnload);

        if (stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
          std::cerr << "unload failed on plugin " << op->getIdentifier() << std::endl;
          return;
        }
      }


      /// handler for preparing to read in a chunk of XML from the cache, set up context to do this
      void PluginCache::beginXmlParsing(Plugin *p) {
        _currentPlugin = dynamic_cast<ImageEffectPlugin*>(p);
      }

      /// XML handler : element begins (everything is stored in elements and attributes)       
      void PluginCache::xmlElementBegin(const std::string &el, std::map<std::string, std::string> map) 
      {
        if (el == "apiproperties") {
          return;
        }

        if (el == "context") {
          std::shared_ptr<Descriptor> newContext = gImageEffectHost->makeDescriptor(_currentPlugin->getBinary()->getBundlePath(), _currentPlugin);
          _currentContext = newContext.get();
          _currentPlugin->addContext(map["name"], newContext);
          return;
        }

        if (el == "param" && _currentContext) {
          std::string pname = map["name"];
          std::string ptype = map["type"];

          _currentParam = _currentContext->paramDefine(ptype.c_str(), pname.c_str());
          return;
        }

        if (el == "clip" && _currentContext) {
          std::string cname = map["name"];

          _currentClip = new ClipDescriptor(cname);
          _currentContext->addClip(cname, _currentClip);
          return;
        }

        if (_currentContext && _currentParam) {
          APICache::propertySetXMLRead(el, map, _currentParam->getProperties(), _currentProp);
          return;
        }

        if (_currentContext && _currentClip) {
          APICache::propertySetXMLRead(el, map, _currentClip->getProps(), _currentProp);
          return;
        }

        if (!_currentContext && !_currentParam) {
          APICache::propertySetXMLRead(el, map, _currentPlugin->getDescriptor().getProps(), _currentProp);
          return;
        }

        std::cout << "element " << el << "\n";
        assert(false);
      }

      void PluginCache::xmlCharacterHandler(const std::string &) {
      }

      void PluginCache::xmlElementEnd(const std::string &el) {
        if (el == "param") {
          _currentParam = 0;
        }

        if (el == "context") {
          _currentContext = 0;
        }
      }

      void PluginCache::endXmlParsing() {
        _currentPlugin = 0;
      }

      void PluginCache::saveXML(Plugin *ip, std::ostream &os) const {
        ImageEffectPlugin *p = dynamic_cast<ImageEffectPlugin*>(ip);
        if (p) {
          p->saveXML(os);
        }
      }

      void PluginCache::confirmPlugin(Plugin *p) {
        ImageEffectPlugin *plugin = dynamic_cast<ImageEffectPlugin*>(p);
        if (!plugin) {
          return;
        }
        _plugins.push_back(plugin);

        if (_pluginsByID.find(plugin->getIdentifier()) != _pluginsByID.end()) {
          ImageEffectPlugin *otherPlugin = _pluginsByID[plugin->getIdentifier()];
          if (plugin->trumps(otherPlugin)) {
            _pluginsByID[plugin->getIdentifier()] = plugin;
          }
        } else {
          _pluginsByID[plugin->getIdentifier()] = plugin;
        }

        MajorPlugin maj(plugin);

        if (_pluginsByIDMajor.find(maj) != _pluginsByIDMajor.end()) {
          ImageEffectPlugin *otherPlugin = _pluginsByIDMajor[maj];
          if (plugin->trumps(otherPlugin)) {
            _pluginsByIDMajor[maj] = plugin;
          }
        } else {
          _pluginsByIDMajor[maj] = plugin;
        }
      }

      Plugin *PluginCache::newPlugin(PluginBinary *pb,
        int pi,
        OfxPlugin *pl) {
          ImageEffectPlugin *plugin = new ImageEffectPlugin(*this, pb, pi, pl);
          return plugin;
      }

      Plugin *PluginCache::newPlugin(PluginBinary *pb,
        int pi,
        const std::string &api,
        int apiVersion,
        const std::string &pluginId,
        const std::string &rawId,
        int pluginMajorVersion,
        int pluginMinorVersion) 
      {
        ImageEffectPlugin *plugin = new ImageEffectPlugin(*this, pb, pi, api, apiVersion, pluginId, rawId, pluginMajorVersion, pluginMinorVersion);
        return plugin;
      }

      void PluginCache::dumpToStdOut()
      {
        if (_pluginsByID.empty())
          std::cout << "No Plug-ins Found." << std::endl;

        for(std::map<std::string, ImageEffectPlugin *>::const_iterator it =  _pluginsByID.begin(); it != _pluginsByID.end(); ++it)
        {
          std::cout << "Plug-in:" << it->first << std::endl;
          std::cout << "\t" << "Filepath: " << it->second->getBinary()->getFilePath();
          std::cout<< "(" << it->second->getIndex() << ")" << std::endl;

          std::cout << "Contexts:" << std::endl;
          const std::set<std::string>& contexts = it->second->getContexts();
          for (std::set<std::string>::const_iterator it2 = contexts.begin(); it2 != contexts.end(); ++it2)
            std::cout << "\t* " << *it2 << std::endl;
          const Descriptor& d =  it->second->getDescriptor();
          std::cout << "Inputs:" << std::endl;
          const std::map<std::string, ClipDescriptor*>& inputs = d.getClips();
          for (std::map<std::string, ClipDescriptor*>::const_iterator it2 = inputs.begin(); it2 != inputs.end(); ++it2)
            std::cout << "\t\t* " << it2->first << std::endl;
        }
      }

    } // ImageEffect
      
  } // Host

} // OFX
