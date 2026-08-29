

#ifndef OFX_PROPERTY_SUITE_H
#define OFX_PROPERTY_SUITE_H
#include "ofxCore.h"
// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>

namespace OFX {
	namespace Host {
		namespace Property {
			/// simple function to turn a thing into a std string
			template<class T> inline std::string castToString(T i) {
				std::ostringstream o;
				o << i;
				return o.str();
			}

			/// simple function to turn a string into an int
			inline int stringToInt(const std::string &s) {
				std::istringstream is(s);
				int number;
				is >> number;
				return number;
			}

			/// simple function to turn a string into a double
			inline double stringToDouble(const std::string &s) {
				std::istringstream is(s);
				double number;
				is >> number;
				return number;
			}
			
			// forward declarations
			class Property; 
			class Set;

			/// exception, representing an OfxStatus
			class Exception {
				OfxStatus _stat;

			public:
				/// ctor
				Exception(OfxStatus stat) : _stat(stat)
				{
				}

				/// get the status
				OfxStatus getStatus() const
				{
					return _stat;
				}
			};

			/// type of a property
			enum TypeEnum {
				eNone = -1,
				eInt = 0,
				eDouble = 1,
				eString = 2,
				ePointer = 3
			};

			/// type holder, for integers, used to template up int properties
			struct IntValue { 
				typedef int APIType; ///< C type of the property that is passed across the raw API
				typedef int APITypeConstless;  ///< C type of the property that is passed across the raw API, without any const it
				typedef int Type; ///< Type we actually hold and deal with the propery in everything by the raw API
				typedef int ReturnType; ///< type to return from a function call
				static const TypeEnum typeCode = eInt;
				static int kEmpty;
			};

			/// type holder, for doubles, used to template up double properties
			struct DoubleValue { 
				typedef double APIType;
				typedef double APITypeConstless;
				typedef double Type;
				typedef double ReturnType; ///< type to return from a function call
				static const TypeEnum typeCode = eDouble;
				static double kEmpty;
			};

			/// type holder, for pointers, used to template up pointer properties
			struct PointerValue { 
				typedef void *APIType;
				typedef void *APITypeConstless;
				typedef void *Type;
				typedef void *ReturnType; ///< type to return from a function call
				static const TypeEnum typeCode = ePointer;
				static void *kEmpty;
			};

			/// type holder, for strings, used to template up string properties
			struct StringValue { 
				typedef const char *APIType;
				typedef char *APITypeConstless;
				typedef std::string Type;
				typedef const std::string &ReturnType; ///< type to return from a function call
				static const TypeEnum typeCode = eString;
				static std::string kEmpty;
			};

			/// array representing the names of the various types, in order of TypeEnum
			extern const char *gTypeNames[];

			/// Sits on a property and can override the local property value when a value is being fetched
			/// only one of these can be in any property (as the thing has only a single value).
			class GetHook {
			public :
				/// dtor
				virtual ~GetHook()
				{
				}

				/// We specialise this to do some magic so that it calls get string/int/double/pointer appropriately
				/// this is what is called by the propertytemplate code to fetch values out of a hook.
				template<class T> typename T::ReturnType getProperty(const std::string &name, int index=0) const;

				/// We specialise this to do some magic so that it calls get int/double/pointer appropriately
				/// this is what is called by the propertytemplate code to fetch values out of a hook.
				template<class T> void getPropertyN(const std::string &name, typename T::APIType *values, int count) const;

				/// override this to fetch a single value at the given index.
				virtual const std::string& getStringProperty(const std::string &name, int index = 0) const;
					
				/// override this to fetch a multiple values in a multi-dimension property
				virtual void getStringPropertyN(const std::string &name, const char** values, int count) const;

				/// override this to fetch a single value at the given index.
				virtual int getIntProperty(const std::string &name, int index = 0) const;

				/// override this to fetch a multiple values in a multi-dimension property
				virtual void getIntPropertyN(const std::string &name, int *values, int count) const;

				/// override this to fetch a single value at the given index.
				virtual double getDoubleProperty(const std::string &name, int index = 0) const;

				/// override this to fetch a multiple values in a multi-dimension property
				virtual void getDoublePropertyN(const std::string &name, double *values, int count) const;

				/// override this to fetch a single value at the given index.
				virtual void *getPointerProperty(const std::string &name, int index = 0) const;
				
				/// override this to fetch a multiple values in a multi-dimension property
				virtual void getPointerPropertyN(const std::string &name, void **values, int count) const;

				/// override this to fetch the dimension size.
				virtual int getDimension(const std::string &name) const;

				/// override this to handle a reset(). 
				virtual void reset(const std::string &name);
			};

			/// Sits on a property and is called when the local property is being set.
			/// It notify or notifyN is called whenever the plugin sets a property
			/// Many of these can sit on a property, as various objects will need to know when a property
			/// has been changed. On notification you should fetch properties with a 'raw' call, rather
			/// than the standard calls, as you may be fetching through a getHook and you won't see
			/// the local value that has been shoved into the property.
			class NotifyHook {
			public :
				/// dtor
				virtual ~NotifyHook() {}

				/// override this to be notified when a property changes
				/// \arg name is the name of the property just set
				/// \arg singleValue is whether setProperty on a single index was call, otherwise N properties were set
				/// \arg indexOrN is the index if single value is true, or the count if singleValue is false
				virtual void notify(const std::string &name, bool singleValue, int indexOrN) = 0;
			};

			/// base class for all properties
			class Property {
			protected :
				std::string  _name;                     ///< name of this property
				TypeEnum     _type;                     ///< type of this property
				int          _dimension;                ///< the fixed dimension of this property 
				bool         _pluginReadOnly;           ///< set is forbidden through suite: value may still change between get() calls
				std::vector<NotifyHook *> _notifyHooks; ///< hooks to call whenever the property is set
				GetHook                  *_getHook;     ///< if we are not storing props locally, they are stored via fetching from here

				friend class Set;
			public :
				/// ctor
				Property(const std::string &name,
								 TypeEnum type,
								 int dimension = 1,
								 bool pluginReadOnly=false);
						
				/// copy ctor
				Property(const Property &other);

				/// dtor
				virtual ~Property()
				{
				}
				
				/// is it read only?
				bool getPluginReadOnly() const {return _pluginReadOnly; }

				/// change the state of readonlyness
				void setPluginReadOnly(bool v) {_pluginReadOnly = v;}

				/// override this to return a clone of the property
				virtual Property *deepCopy() = 0;
				
				/// get the name of this property
				const std::string &getName()
				{
					return _name;
				}
				
				/// get the type of this property
				TypeEnum getType()
				{
					return _type;
				}

				/// add a notify hook
				void addNotifyHook(NotifyHook *hook)
				{
					_notifyHooks.push_back(hook);
				}
				
				/// set the get hook
				void setGetHook(GetHook *hook)
				{
					_getHook = hook;
				}
				
				/// call notify on the contained notify hooks
				void notify(bool single, int indexOrN);

				// get the current dimension of this property
				virtual int getDimension() const = 0;

				/// get the fixed dimension of this property
				int getFixedDimension() const {
					return _dimension;
				}

				/// are we a fixed dim property
				bool isFixedSize() const 
				{
					return _dimension != 0;
				}

				/// reset this property to the default
				virtual void reset() = 0;

				// get a string representing the value of this property at element nth
				virtual std::string getStringValue(int nth) = 0;
			};
			
			/// this represents a generic property.
			/// template parameter T is the type descriptor of the
			/// type of property to model.  the class holds an internal _value vector which can be used
			/// to store the values.  if set and get hooks are installed, these will be called instead
			/// of using this variable.
			/// Make sure that T::ReturnType is const if appropriate, as no extra qualifiers are applied here.
			template<class T>
			class PropertyTemplate : public Property
			{
			public :
				typedef typename T::Type Type; 
				typedef typename T::ReturnType ReturnType; 
				typedef typename T::APIType APIType;
				
			protected :
				/// this is the present value of the property
				std::vector<Type> _value;

				/// this is the default value of the property
				std::vector<Type> _defaultValue;

			public :
				/// constructor
				PropertyTemplate(const std::string &name,
												 int dimension,
												 bool pluginReadOnly,
												 APIType defaultValue);

				PropertyTemplate(const PropertyTemplate<T> &pt);

				PropertyTemplate<T> *deepCopy() {
					return new PropertyTemplate(*this);
				}

				virtual ~PropertyTemplate()
				{
				}

				/// get the vector
				const std::vector<Type> &getValues()
				{
					return _value;
				}

				// get multiple values
				void getValueN(APIType *value, int count) const;

#ifdef _MSC_VER
#pragma warning( disable : 4181 )
#endif
				/// get one value
				const ReturnType getValue(int index=0) const;

				/// get one value, without going through the getHook
				const ReturnType getValueRaw(int index=0) const;

#ifdef _MSC_VER
#pragma warning( default : 4181 )
#endif
				// get multiple values, without going through the getHook
				void getValueNRaw(APIType *value, int count) const;

				/// set one value
				void setValue(const Type &value, int index=0);

				/// set multiple values
				void setValueN(const APIType *value, int count);

				/// reset 
				void reset();
				
				/// get the size of the vector
				int getDimension() const;
				
				/// return the value as a string
				inline std::string getStringValue(int idx) {
					return castToString(_value[idx]);
				}
			};


			typedef PropertyTemplate<IntValue>     Int;     /// Our int property
			typedef PropertyTemplate<DoubleValue>  Double;  /// Our double property
			typedef PropertyTemplate<StringValue>  String;  /// Our string property
			typedef PropertyTemplate<PointerValue> Pointer; /// Our pointer property

			/// A class that is used to initialise a property set. Feed in an array of these to
			/// a property and it will construct a bunch of properties. Terminate such an array
			/// with an empty (all zero) set.
			struct PropSpec {
				const char *name;          ///< name of the property 
				TypeEnum type;             ///< type
				int dimension;             ///< fixed dimension of the property, set to zero if variable dimension
				bool readonly;             ///< is the property plug-in read only
				const char *defaultValue;  ///< Default value as a string. Pointers are ignored and always null.
			};
			static const PropSpec propSpecEnd = {0, eNone, 0, false, 0};

			/// A std::map of properties by name
			typedef std::map<std::string, Property *> PropertyMap;


			//................................................................................
			/// Class that holds a set of properties and manipulates them
			/// The 'fetch' methods return a property object.
			/// The 'get' methods return a property value
			class Set {
			private :
				static const int kMagic = 0x12082007; ///< magic number for property sets, and Connie's birthday :-)
				const int   _magic; ///< to check for handles being nice

			protected :
				PropertyMap _props; ///< Our properties.

				/// chained property set, which is read only
				/// these are searched on a get if not found 
				/// on a local search
				Set *_chainedSet;

				/// hide assignment
				void operator=(const Set &);

				/// set a particular property
				template<class T> void setProperty(const std::string &property, int index, const typename T::Type &value);

				/// set the first N of a particular property
				template<class T> void setPropertyN(const std::string &property, int count, const typename T::APIType *value);

				/// get a particular property
				template<class T> typename T::ReturnType getProperty(const std::string &property, int index)  const;

				/// get the first N of a particular property
				template<class T> void getPropertyN(const std::string &property, int index, typename T::APIType *v)  const;

				/// get a particular property without going through any getHook
				template<class T> typename T::ReturnType getPropertyRaw(const std::string &property, int index)  const;

				/// get a particular property without going through any getHook
				template<class T> void getPropertyRawN(const std::string &property, int count, typename T::APIType *v)  const;

			public :
				/// take an array of of PropSpecs (which must be terminated with an entry in which
				/// ->name is null), and turn these into a Set
				explicit Set(const PropSpec *);

				/// deep copies the property set
				explicit Set(const Set &);

				/// empty ctor
				explicit Set();

				/// destructor
				virtual ~Set();

				/// adds a bunch of properties from PropSpec
				void addProperties(const PropSpec *);
				
				/// add one new property
				void createProperty(const PropSpec &s);

				/// add one new property
				void addProperty(Property *prop);

				/// set the chained property set
				void setChainedSet(Set *s) {_chainedSet = s;}

				/// grab the internal properties map
				const PropertyMap &getProperties() const
				{
					return _props;
				}

				/// set the get hook for a particular property.  users may need to call particular
				/// specialised versions of this.
				void setGetHook(const std::string &s, GetHook *ghook) const;

				/// add a set hook for a particular property.  users may need to call particular
				/// specialised versions of this.
				void addNotifyHook(const std::string &name, NotifyHook *hook) const;
								
				/// Fetchs a pointer to a property of the given name, following the property chain if the
				/// 'followChain' arg is not false.
				Property *fetchProperty(const std::string &name, bool followChain = false) const;

				/// get property with the particular name and type.  if the property is 
				/// missing or is of the wrong type, return an error status.  if this is a sloppy
				/// property set and the property is missing, a new one will be created of the right
				/// type
				template<class T> bool fetchTypedProperty(const std::string &name, T *&prop, bool followChain = false) const;

				/// retrieve the nameed string property
				String *fetchStringProperty(const std::string &name,  bool followChain = false) const;

				/// retrieve the named double property
				Double *fetchDoubleProperty(const std::string &name,  bool followChain = false) const;

				/// retrieve the named double property
				Pointer *fetchPointerProperty(const std::string &name, bool followChain = false) const;

				/// retrieve the named double property
				Int *fetchIntProperty(const std::string &name, bool followChain = false) const;



				/// get a particular int property without fetching via a get hook, useful for notifies
				int getIntPropertyRaw(const std::string &property, int index = 0) const;
				
				/// get a particular double property without fetching via a get hook, useful for notifies
				double getDoublePropertyRaw(const std::string &property, int index = 0) const;

				/// get a particular pointer property without fetching via a get hook, useful for notifies
				void *getPointerPropertyRaw(const std::string &property, int index = 0) const;

				/// get a particular string property
				const std::string &getStringPropertyRaw(const std::string &property, int index = 0) const;
								
				/// get the value of a particular string property
				const std::string &getStringProperty(const std::string &property, int index = 0) const;
				
				/// get the value of a particular int property
				int getIntProperty(const std::string &property, int index = 0) const;
				
				/// get the value of a particular double property
				void getIntPropertyN(const std::string &property,  int *v, int N) const;

				/// get the value of a particular double property
				double getDoubleProperty(const std::string &property, int index = 0) const;

				/// get the value of a particular double property
				void getDoublePropertyN(const std::string &property,  double *v, int N) const;

				/// get the value of a particular pointer property
				void *getPointerProperty(const std::string &property, int index = 0) const;



				/// set a particular string property without fetching via a get hook, useful for notifies
				void setStringProperty(const std::string &property, const std::string &value, int index = 0);

				/// get a particular int property
				void setIntProperty(const std::string &property, int v, int index = 0);

				/// get a particular double property
				void setIntPropertyN(const std::string &property, const int *v, int N);
				
				/// get a particular double property
				void setDoubleProperty(const std::string &property, double v, int index = 0);

				/// get a particular double property
				void setDoublePropertyN(const std::string &property, const double *v, int N);

				/// get a particular double property
				void setPointerProperty(const std::string &property,  void *v, int index = 0);
				


				/// get the dimension of a particular property
				int getDimension(const std::string &property) const;

				/// is the given string one of the values of a multi-dimensional string prop
				/// this returns a non negative index if it is found, otherwise, -1
				int findStringPropValueIndex(const std::string &propName,
																		 const std::string &propValue) const;


				/// get a handle on this object for passing to the C API
				OfxPropertySetHandle getHandle() const
				{
					return (OfxPropertySetHandle)this;
				}

				/// is this a nice property set, or a dodgy pointer passed back to us
				bool verifyMagic() { return _magic == kMagic; }
			};

			
			/// return the OFX function suite that manages properties
			const void *GetSuite(int version);
		}
	}
}

#endif
