

#include "ofxCore.h"
#include "CoreFoundation/CoreFoundation.h"



typedef OfxPlugin *(OfxGetPluginFunc)(int nth);
typedef int (OfxGetNumberOfPluginsFunc)(void);

int 
main(int argc, char *argv[])
{
  CFURLRef bundleURL;
  CFBundleRef myBundle;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <plugin>\n", argv[0]);
    exit(1);
  }
  CFStringRef bundlePath = CFStringCreateWithCString(kCFAllocatorDefault,
						     argv[1],
						     kCFStringEncodingASCII);

  // Make a CFURLRef from the CFString representation of the 
  // bundle's path.
  bundleURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, 
					    bundlePath,
					    kCFURLPOSIXPathStyle,
					    true );

  // Make a bundle instance using the URLRef.
  myBundle = CFBundleCreate( kCFAllocatorDefault, bundleURL );

  if(myBundle) {
    // Value returned from the loaded function.
    //long result;
  
    OfxGetNumberOfPluginsFunc *nPluginsFunc = (OfxGetNumberOfPluginsFunc *)CFBundleGetFunctionPointerForName(myBundle, CFSTR("OfxGetNumberOfPlugins") );
    OfxGetPluginFunc *getPlugin = (OfxGetPluginFunc *)CFBundleGetFunctionPointerForName(myBundle, CFSTR("OfxGetPlugin") );

    // If the function was found, call it with a test value. 
    if (nPluginsFunc && getPlugin) {
      // This should add 1 to whatever was passed in
      int nP =  nPluginsFunc();


      printf("Sucessfully loaded '%s', containing %d %s\n", argv[1], nP, (nP == 1 ? "plugin" : "plugins"));    

      for(int i = 0; i < nP; i++) {
	// get a plugin
	OfxPlugin *plugin = getPlugin(i); 
	if(plugin) {
	  printf("\tFound plugin...\n\t\tAPI = %s (%d)\n\t\tid = %s (%d.%d)\n",
		 plugin->pluginApi, plugin->apiVersion,
		 plugin->pluginIdentifier, plugin->pluginVersionMajor, plugin->pluginVersionMinor);

	  plugin->mainEntry(kOfxActionLoad, NULL, NULL, NULL);
	  plugin->mainEntry(kOfxActionUnload, NULL, NULL, NULL);
	}
	else
	  fprintf(stderr, "fetching %dth plugin returned NULL\n", i);
      }
    }
    else
      fprintf(stderr, "Failed to find symbols OfxGetPlugin or OfxGetNumberOfPlugins\n");
  }
  else
    fprintf(stderr, "Failed to load bundle %s\n", argv[1]);

  // Any CF objects returned from functions with "create" or 
  // "copy" in their names must be released by us!
  CFRelease( bundleURL );
}
