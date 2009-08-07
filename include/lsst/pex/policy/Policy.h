// -*- lsst-c++ -*-
#ifndef LSST_PEX_POLICY_POLICY_H
#define LSST_PEX_POLICY_POLICY_H

#include <string>
#include <vector>
#include <list>
#include <map>

#include "lsst/daf/base/Citizen.h"
#include "lsst/daf/base/Persistable.h"
#include "lsst/daf/base/PropertySet.h"
#include "lsst/pex/policy/exceptions.h"
#include <boost/shared_ptr.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

namespace lsst {
namespace pex {
namespace policy {

// forward declaration
class PolicySource;
class PolicyFile;
class Dictionary;

namespace fs = boost::filesystem;
namespace pexExcept = lsst::pex::exceptions;
namespace dafBase = lsst::daf::base;

#define POL_GETSCALAR(name, type, vtype) \
    try { \
        return _data->get<type>(name); \
    } catch (pexExcept::NotFoundException&) {   \
        throw LSST_EXCEPT(NameNotFound, name);  \
    } catch (dafBase::TypeMismatchException&) { \
        throw LSST_EXCEPT(TypeError, name, std::string(typeName[vtype])); \
    } catch (boost::bad_any_cast&) { \
        throw LSST_EXCEPT(TypeError, name, std::string(typeName[vtype])); \
    }

#define POL_GETLIST(name, type, vtype) \
    try { \
        return _data->getArray<type>(name); \
    } catch (pexExcept::NotFoundException&) {   \
        throw LSST_EXCEPT(NameNotFound, name);  \
    } catch (dafBase::TypeMismatchException&) { \
        throw LSST_EXCEPT(TypeError, name, std::string(typeName[vtype])); \
    } catch (boost::bad_any_cast&) { \
        throw LSST_EXCEPT(TypeError, name, std::string(typeName[vtype])); \
    } 


/**
 * @brief  a container for holding hierarchical configuration data in memory.
 *
 * A policy is a set of named parameters that can be used to configure
 * the internal data and behavior of an object within an application.  An
 * important feature Policy objects is that the parameters can be
 * loaded in from a file.  Thus, it allows applications fine-grained
 * control of objects even if much of the configuration parameters they
 * provide are normally set to defaults and otherwise do not change.
 *
 * The Policy interface allows an application to pull out parameter values 
 * by name.  Typically, the application "knows" the names it needs from a 
 * Policy to configure itself--that is, these names and the use of their 
 * values are hard-coded into the application.  The application simply calls 
 * one of the get methods to retrieve a typed value for the parameter.  
 * (Nevertheless, if necessary, the parameter names contained in a policy 
 * can be retrieved via the \c names() member function.)
 * 
 * Policy parameters values are restricted to a limited set of types to ensure 
 * simple, well-defined ASCII text serialization format.  These types are 
 * In particular, a Policy parameter can be one of:
 *  - integer (int not long)
 *  - double
 *  - string
 *  - boolean (bool)
 *  - Policy
 *  - PolicyFile, a reference to file containing additional Policy data
 *  - arrays of any of the above
 *
 * As implied by the inclusion of Policy as a value type, a Policy can be 
 * hierarchical (like a PropertySet).  Values deep within the hierarchy 
 * can be retrieved via a hiearchical name.  Such a name is made up of 
 * name fields delimited by dots (.) where each name field represents a 
 * level of the hierarchy.  The first field represents the top level of 
 * the hierarchy.  If a given name does not resolve to value, a NameNotFound
 * exception is thrown.  If one expects a different value type for a name 
 * than what is actually stored, (e.g. one calls \c getInt(name) for a 
 * parameter that is actually a string), a TypeError exception is thrown.
 * 
 * A hierarchical Policy allows an application to configure a whole 
 * hierarchy of objects, even if the object classes (and their 
 * associated policy parameters) were developed by different people.  In
 * particular, if an application that is configuring one of the objects it 
 * uses, it can either pull out the relevent values directly by their 
 * hierarchical names, or if that object supports configuration from a 
 * Policy, it can pull all of the values for the object by retrieving a 
 * Policy (via \c getPolicy(name) ) and passing it to the object.  
 * 
 * It is important to note that parameter names relative the Policy that 
 * contains them.  For example, suppose you have a parameter accessible via 
 * the name "foo.bar.zub".  This implies that the name "foo.bar" will 
 * resolve to a Policy object.  You can retrieve the "zub" parameter from 
 * that "sub-Policy" object by asking for "zub".  
 * 
 * \section secPolicyLoading Loading a Policy File
 *
 * One can read Policy data via the constructors that take a file name in 
 * some form as an input argument; however the prefered manner is via one
 * of the createPolicy() functions.  The latter is able to intelligently 
 * differentiate between simple Policy file and a Dictionary file.  In 
 * either case, the formats supported would be restricted by default to 
 * those currently built into this package.  Support for other formats 
 * can be plugged in using the PolicyFile and SupportFormats classes.  
 *
 * For more information about about Policy files, file formats, and 
 * dictionaries, see the \ref main "package overview" as well as the 
 * PolicyFile and Dictionary class descriptions.
 *
 * \section secPolicyDefaults Default Policy Data
 *
 * When an object using a Policy fails to find a parameter it was expecting,
 * it is a little inelegant to provide a default hard-coded in the object's
 * implementation, <em>by design</em>.  Instead it is recommended that 
 * defaults be loaded from another Policy.  The intended way to do this is 
 * to load defaults via a DefaultPolicyFile (which can located a policy file
 * from any EUPS-setup installation directory) and to merge them into to the
 * primary Policy instance via the mergeDefaults() function.  
 * 
 * \section secPolicyVer Version Notes
 *
 * With version 3.1, support for the JSON format was dropped.  
 * 
 * After version 3.2, Policy's internal implementation was changed to be 
 * a wrapper around PropertySet.  Prior to this, it used its own internal 
 * data map.  The API (which had been optimized for the old implementation), 
 * remained largely unchanged for the purposes of backward compatibility.  
 * The changes include (1) dropping support for providing default values to 
 * the get*() functions, (2) returning array values by value rather than 
 * reference, and (3) added functions for greater consistancy with 
 * PropertySet.
 *
 * After 3.3.3, loading default data (including from Dictionarys) was 
 * improved.  This included adding mergeDefaults()
 */
class Policy : public dafBase::Citizen, public dafBase::Persistable {
public:

    typedef boost::shared_ptr<Policy> Ptr;
    typedef boost::shared_ptr<PolicyFile> FilePtr;
    typedef boost::shared_ptr<const Policy> ConstPtr;
    typedef std::vector<bool> BoolArray;
    typedef std::vector<int> IntArray;
    typedef std::vector<double> DoubleArray;
    typedef std::vector<std::string> StringArray;
    typedef std::vector<Ptr> PolicyPtrArray;
    typedef std::vector<FilePtr> FilePtrArray;
    typedef std::vector<ConstPtr> ConstPolicyPtrArray;

    /**
     * an enumeration for the supported policy types
     */
    enum ValueType {
        UNDEF,
        BOOL,
        INT,
        DOUBLE,
        STRING,
        POLICY,
        FILE
    };

    /**
     * c-string forms for the supported value types.  The ValueType 
     * enum values can be used a indexes into the array to return the 
     * name of the corresping type.  
     */
    static const char * const typeName[];

    /**
     * Create an empty policy
     */
    Policy();

    //@{
    /**
     * Create a Policy from a named file
     * @param filePath   the file name, which can be a file path
     */
    explicit Policy(const std::string& filePath);
    explicit Policy(const char *filePath);
    //@}

    /**
     * Create a Policy from a named file
     * @param file       the file name, represented as a PolicyFile
     */
    explicit Policy(const PolicyFile& file);

    /**
     * Create a default Policy from a Dictionary.  If the Dictionary 
     *   references files containing dictionaries for sub-Policies, an
     *   attempt is made to open them and extract the default data.
     *
     * Note:  validation is not implemented yet.
     *
     * @param validate    if true, a (shallow) copy of the Dictionary will be 
     *                      held onto by this Policy and used to validate 
     *                      future updates.  
     * @param dict        the Dictionary file load defaults from
     * @param repository  the directory to look for dictionary files referenced
     *                      in \c dict.  The default is the current directory.
     */
    Policy(bool validate, const Dictionary& dict, 
           const fs::path& repository="");

    /**
     * copy a Policy.  
     * @param pol   the policy to copy
     * @param deep  if true, do a deep copy.  Otherwise, Policy data will
     *                 be shared.
     */
    Policy(Policy& pol, bool deep=false);

    /**
     * deep-copy a Policy.  
     */
    Policy(const Policy& pol);

    //@{
    /**
     * create a Policy from a file.  The caller is responsible for deleteing
     * the returned value.  This is the preferred way to obtain a Policy file 
     * if you don't care to know if the input file is an actual policy 
     * file or a policy dictionary.  If it turns out to be a dictionary 
     * file, the defined defaults will be loaded into the return policy.  
     * @param input       the input file or stream to load data from.
     * @param doIncludes  if true, any references found to external Policy 
     *                      files will be resolved into sub-policy values.  
     *                      The files will be looked for in a directory 
     *                      relative the current directory.
     * @param validate    if true and the input file is a policy dictionary,
     *                      it will be given to the returned policy and 
     *                      used to validate future updates to the Policy.
     */
    static Policy *createPolicy(PolicySource& input, bool doIncludes=true, 
                                bool validate=false);
    static Policy *createPolicy(const std::string& input, bool doIncludes=true,
                                bool validate=false);
    //@}

    //@{
    /**
     * create a Policy from a file.  The caller is responsible for deleteing
     * the returned value.  This is the preferred way to obtain a Policy file 
     * if you don't care to know if the input file is an actual policy 
     * file or a policy dictionary.  If it turns out to be a dictionary 
     * file, the defined defaults will be loaded into the return policy.  
     * @param input       the input file or stream to load data from.
     * @param repos       a directory to look in for the referenced files.  
     *                      Only when the name of the file to be included is an
     *                      absolute path will this.  
     * @param validate    if true and the input file is a policy dictionary,
     *                      it will be given to the returned policy and 
     *                      used to validate future updates to the Policy.
     */
    static Policy *createPolicy(PolicySource& input, const fs::path& repos, 
                                bool validate=false);
    static Policy *createPolicy(PolicySource& input, const std::string& repos, 
                                bool validate=false);
    static Policy *createPolicy(PolicySource& input, const char *repos, 
                                bool validate=false);
    static Policy *createPolicy(const std::string& input, 
                                const fs::path& repos, bool validate=false);
    static Policy *createPolicy(const std::string& input, 
                                const std::string& repos, bool validate=false);
    static Policy *createPolicy(const std::string& input, const char *repos, 
                                bool validate=false);
    //@}

    /**
     * A template-ized way to get the ValueType.
     */
    template <class T> static ValueType getValueType();

    /**
     * destroy this policy
     */
    virtual ~Policy();

    //@{
    /**
     * load the names of parameters into a given list.  \c names() returns
     * all names, \c paramNames() only returns the names that resolve to
     * non-Policy and non-PolicyFile type parameters, \c policyNames() 
     * only returns the Policy names, and fileNames() only returns PolicyFile
     * names.  These versions are provided for backward compatibility but are
     * deprecated; use the versions that return vector values.  
     * 
     * @param names         the list object to be loaded
     * @param topLevelOnly  if true, only parameter names at the top of the 
     *                         hierarchy will be returned; no hierarchical 
     *                         names will be included.
     * @param append        if false, the contents of the given list will 
     *                         be erased before loading the names.  
     * @return int  the number of names added
     */
    int names(std::list<std::string>& names, 
              bool topLevelOnly=false, 
              bool append=false) const;                      // inlined below
    int paramNames(std::list<std::string>& names, 
                   bool topLevelOnly=false, 
                   bool append=false) const;                 // inlined below
    int policyNames(std::list<std::string>& names, 
                    bool topLevelOnly=false, 
                    bool append=false) const;                // inlined below
    int fileNames(std::list<std::string>& names, 
                  bool topLevelOnly=false, 
                  bool append=false) const;                  // inlined below
    //@}

    //@{
    /**
     * return the names of parameters.  \c names() returns
     * all names, \c paramNames() only returns the names that resolve to
     * non-Policy and non-PolicyFile type parameters, \c policyNames() 
     * only returns the Policy names, and fileNames() only returns PolicyFile
     * names.  
     * 
     * @param topLevelOnly  if true, only parameter names at the top of the 
     *                         hierarchy will be returned; no hierarchical 
     *                         names will be included.
     */
    StringArray names(bool topLevelOnly=false) const;        // inlined below
    StringArray paramNames(bool topLevelOnly=false) const;   // inlined below
    StringArray policyNames(bool topLevelOnly=false) const;  // inlined below
    StringArray fileNames(bool topLevelOnly=false) const;    // inlined below
    //@}

    /**
     * return true if it appears that this Policy actually contains dictionary
     * definition data.
     */
    bool isDictionary() {  return exists("definitions");  }

    /**
     * return the number of values currently associated with a given name
     * @param name   the (possibly) hierarchical name of the property of 
     *                  interest.
     */
    size_t valueCount(const std::string& name) const;        // inlined below

    /**
     * return true if multiple values can be retrieved via the given name.
     * False is returned if the name does not exist.  This is equivalent to
     * (valueCount(name) > 1).
     * @param name   the (possibly) hierarchical name of the property of 
     *                  interest.
     */
    bool isArray(const std::string& name) const;             // inlined below

    /**
     * return true if a value exists in this policy for the given name. 
     * This is semantically equivalent to (valuecount(name) > 0) (though
     * its implementation is slightly more efficient).
     * @param name   the (possibly) hierarchical name of the property of 
     *                  interest.
     */
    bool exists(const std::string& name) const;              // inlined below

    /**
     * return true if the value pointed to by the given name is a boolean
     */
    bool isBool(const std::string& name) const;               // inlined below

    /**
     * return true if the value pointed to by the given name is an integer
     */
    bool isInt(const std::string& name) const;               // inlined below

    /**
     * return true if the value pointed to by the given name is a double
     */
    bool isDouble(const std::string& name) const;            // inlined below

    /**
     * return true if the value pointed to by the given name is a string
     */
    bool isString(const std::string& name) const;            // inlined below

    /**
     * return true if the value pointed to by the given name is a Policy
     */
    bool isPolicy(const std::string& name) const;            // inlined below

    /**
     * return true if the value pointed to by the given name is a PolicyFile
     */
    bool isFile(const std::string& name) const;              // inlined below

    /**
     * return the type information for the underlying type associated with
     * a given name.  This is equivalent to typeOf() and is provided for
     * backward compatibility.
     */
    const std::type_info& getTypeInfo(const std::string& name) const; //inlined

    /**
     * return the type information for the underlying type associated with
     * a given name.  This is equivalent to getTypeInfo() and is provided 
     * for consistency with PropertySet.
     */
    const std::type_info& typeOf(const std::string& name) const;     // inlined

    /**
     * return the ValueType enum identifier for the underlying type associated 
     * with a given name.  If a parameter with that name has not been set, 
     * Policy::UNDEF is returned.  
     */
    ValueType getValueType(const std::string& name) const;

    /**
     * return a string name for the type associated with the parameter of
     * a given name.  If a parameter with that name has not been set, the 
     * returned string will be "undefined".  
     */
    const char *getTypeName(const std::string& name) const {
        try {  return typeName[getValueType(name)]; }
        catch (NameNotFound&) { return typeName[UNDEF]; }
    }

    /**
     * Template-ized version of getInt, getPolicy, etc.  General case is
     * disallowed, but known types are implemented.
     */
    template <class T> T getValue(const std::string& name) const;

    /**
     * Template-ized version of getIntArray, getPolicyPtrArray, etc.  General
     * case is disallowed, but known types are implemented.
     */
    template <class T> std::vector<T> getValueArray(const std::string& name) const;

    /**
     * return a "sub-Policy" identified by a given name.  
     * @param name     the name of the parameter.  This can be a hierarchical
     *                    name with fields delimited with "."
     * @exception NameNotFound  if no value is associated with the given name.
     * @exception TypeError     if the value associated the given name is not
     *                             a Policy type.  
     */
    ConstPtr getPolicy(const std::string& name) const;       // inlined below
    Ptr getPolicy(const std::string& name);                  // inlined below

    /**
     * return a PolicyFile (a reference to a file with "sub-Policy" data) 
     * identified by a given name.  
     * @param name     the name of the parameter.  This can be a hierarchical
     *                    name with fields delimited with "."
     * @exception NameNotFound  if no value is associated with the given name.
     * @exception TypeError     if the value associated the given name is not
     *                             a Policy type.  
     */
    FilePtr getFile(const std::string& name) const;

    /**
     * return a boolean value associated with the given name.  If the 
     * underlying value is an array, only the last value added will be 
     * returned.  
     * @param name     the name of the parameter.  This can be a hierarchical
     *                    name with fields delimited with "."
     * @exception NameNotFound  if no value is associated with the given name.
     * @exception TypeError     if the value associated the given name is not
     *                             a boolean type.  
     */
    bool getBool(const std::string& name) const { 
        POL_GETSCALAR(name, bool, BOOL) 
    }

    /**
     * return an integer value associated with the given name.  If the 
     * underlying value is an array, only the last value added will be 
     * returned.  
     * @param name     the name of the parameter.  This can be a hierarchical
     *                    name with fields delimited with "."
     * @exception NameNotFound  if no value is associated with the given name.
     * @exception TypeError     if the value associated the given name is not
     *                             an integer type.  
     */
    int getInt(const std::string& name) const { 
        POL_GETSCALAR(name, int, INT) 
    }

    /**
     * return a double value associated with the given name.  If the 
     * underlying value is an array, only the last value added will be 
     * returned.  
     * @param name     the name of the parameter.  This can be a hierarchical
     *                    name with fields delimited with "."
     * @exception NameNotFound  if no value is associated with the given name.
     * @exception TypeError     if the value associated the given name is not
     *                             a double type.  
     */
    double getDouble(const std::string& name) const { 
        POL_GETSCALAR(name, double, DOUBLE) 
    }

    /**
     * return a string value associated with the given name .  If the 
     * underlying value is an array, only the last value added will be 
     * returned.  
     * @param name     the name of the parameter.  This can be a hierarchical
     *                    name with fields delimited with "."
     * @exception NameNotFound  if no value is associated with the given name.
     * @exception TypeError     if the value associated the given name is not
     *                             an integer type.  
     */
    const std::string getString(const std::string& name) const { 
        POL_GETSCALAR(name, std::string, STRING) 
    }

    /**
     * return an array of Policy pointers associated with the given name.  
     * 
     * Adding an element to the returned array (using push_back()) is 
     * equivalent adding a value with add(const string&, const Policy::Ptr&). 
     * 
     * @param name     the name of the parameter.  This can be a hierarchical
     *                    name with fields delimited with "."
     * @exception NameNotFound  if no value is associated with the given name.
     * @exception TypeError     if the value associated the given name is not
     *                             a Policy type.  
     */
    PolicyPtrArray getPolicyArray(const std::string& name) const;

    /**
     * return an array of PolicyFile pointers associated with the given name.  
     * 
     * Adding an element to the returned array (using push_back()) is 
     * equivalent adding a value with 
     * add(const string&, const Policy::FilePtr&). 
     * 
     * @param name     the name of the parameter.  This can be a hierarchical
     *                    name with fields delimited with "."
     * @exception NameNotFound  if no value is associated with the given name.
     * @exception TypeError     if the value associated the given name is not
     *                             a Policy type.  
     */
    FilePtrArray getFileArray(const std::string& name) const; 

    /**
     * return an array of booleans associated with the given name
     * @param name     the name of the parameter.  This can be a hierarchical
     *                    name with fields delimited with "."
     * @exception NameNotFound  if no value is associated with the given name.
     * @exception TypeError     if the value associated the given name is not
     *                             an boolean type.  
     */
    BoolArray getBoolArray(const std::string& name) const;  // inlined
    
    /**
     * return an array of integers associated with the given name
     * @param name     the name of the parameter.  This can be a hierarchical
     *                    name with fields delimited with "."
     * @exception NameNotFound  if no value is associated with the given name.
     * @exception TypeError     if the value associated the given name is not
     *                             an integer type.  
     */
    IntArray getIntArray(const std::string& name) const;    // inlined
    
    /**
     * return an array of doubles associated with the given name
     * @param name     the name of the parameter.  This can be a hierarchical
     *                    name with fields delimited with "."
     * @exception NameNotFound  if no value is associated with the given name.
     * @exception TypeError     if the value associated the given name is not
     *                             a double type.  
     */
    DoubleArray getDoubleArray(const std::string& name) const; // inlined
    
    /**
     * return an array of string pointers associated with the given name
     * @param name     the name of the parameter.  This can be a hierarchical
     *                    name with fields delimited with "."
     * @exception NameNotFound  if no value is associated with the given name.
     * @exception TypeError     if the value associated the given name is not
     *                             a string type.  
     */
    StringArray getStringArray(const std::string& name) const;  //inlined

    //@{
    /**
     * add a value with a given name.  
     * Any previous value set with the same name will be overwritten.  In 
     * particular, if the property previously pointed to an array of values,
     * all those values will be forgotten.
     * Note that \code set(const string&, const string&) \endcode and 
     * \code set(const string&, const char *) \endcode are equivalent.
     * @param name       the name of the parameter.  This can be a hierarchical
     *                    name with fields delimited with "."
     * @param value      the value--int, double, string or Policy--to 
     *                    associate with the name.  
     * @exception TypeError  if this Policy is controlled by a Dictionary but 
     *                    the value type does not match the definition 
     *                    associated with the name. 
     */
    void set(const std::string& name, const Ptr& value);      // inlined below
    void set(const std::string& name, const FilePtr& value);  // inlined below
    void set(const std::string& name, bool value);            // inlined below
    void set(const std::string& name, int value);             // inlined below
    void set(const std::string& name, double value);          // inlined below
    void set(const std::string& name, const std::string& value);  // inlined
    void set(const std::string& name, const char *value);     // inlined below
    //@}

    //@{
    /**
     * add a value to an array of values with a given name.  
     * If a value was previously set using the set() function, that previous
     * value will be retained as the first value of the array.
     * Note that \code add(const string&, const string&) \endcode and 
     * \code add(const string&, const char *) \endcode are equivalent.
     * @param name       the name of the parameter.  This can be a hierarchical
     *                    name with fields delimited with "."
     * @param value      the value--int, double, string or Policy--to 
     *                    associate with the name.  
     * @exception TypeError  if the existing array of values is not of the 
     *                    requested type, or 
     *                    if this Policy is controlled by a Dictionary but 
     *                    the value type does not match the definition 
     *                    associated with the name. 
     */
    void add(const std::string& name, const Ptr& value);      // inlined below
    void add(const std::string& name, const FilePtr& value);  // inlined below
    void add(const std::string& name, bool value);            // inlined below
    void add(const std::string& name, int value);             // inlined below
    void add(const std::string& name, double value);          // inlined below
    void add(const std::string& name, const std::string& value);    // inlined
    void add(const std::string& name, const char *value);     // inlined below
    //@}

    /**
     * recursively replace all PolicyFile values with the contents of the 
     * files they refer to.  The type of a parameter containing a PolicyFile
     * will consequently change to a Policy upon successful completion.  If
     * the value is an array, all PolicyFiles in the array must load without
     * error before the PolicyFile values themselves are erased 
     */
    void loadPolicyFiles() { loadPolicyFiles(fs::path()); }

    /**
     * \copydoc loadPolicyFiles() (unless 
     * strict=true; see arguments below).  
     * @param repository  a directory to look in for the referenced files.  
     *                      Only when the name of the file to be included is an
     *                      absolute path will this.  If empty or not provided,
     *                      the directorywill be assumed to be the current one.
     * @param strict      if true, throw an exception if an error occurs 
     *                      while reading and/or parsing the file.  Otherwise,
     *                      an unrecoverable error will result in the failing
     *                      PolicyFile being replaced with an incomplete
     *                      Policy.  
     */
    virtual void loadPolicyFiles(const fs::path& repository,bool strict=false);

    /**
     * use the values found in the given policy as default values for 
     * parameters not specified in this policy.  This function will iterate
     * through the parameter names in the given policy, and if the name is 
     * not found in this policy, the value from the given one will by copied 
     * into this one.  No attempt is made to add match the number of values 
     * available per name.  
     * @param defaultPol   the policy to pull default values from.  This may 
     *                        be a Dictionary; if so, the default values will 
     *                        drawn from the appropriate default keyword.
     * @return int         the number of parameter names copied over
     */
    int mergeDefaults(const Policy& defaultPol);

    /**
     * return a string representation of the value given by a name.  The
     * string "<null>" is printed if the name does not exist.
     * @param name     the name of the parameter to string-ify
     * @param indent  a string to prepend each line with.  If the string 
     *                 includes embedded newline characters, each line 
     *                 should be preceded by this indent string.
     */
    virtual std::string str(const std::string& name, 
                            const std::string& indent="") const;

    /**
     * print the contents of this policy to an output stream.  This 
     * is mainly intended for debugging purposes.  
     * @param out     the output stream to write contents to
     * @param label   a labeling string to lead the listing with.
     * @param indent  a string to prepend each line with.
     */
    virtual void print(std::ostream& out, const std::string& label="Policy", 
                       const std::string& indent="") const;

    /**
     * convert the entire contents of this policy to a string.  This 
     * is mainly intended for debugging purposes.  
     */
    std::string toString() const;

    /**
     * return the internal policy data as a PropertySet pointer.  All
     * sub-policy data will appear as PropertySets.
     */
    dafBase::PropertySet::Ptr asPropertySet();             // inlined below

protected:

    /**
     * use a PropertySet as the data for a new Policy object
     */
    Policy(const dafBase::PropertySet::Ptr ps) 
        : Citizen(typeid(this)), dafBase::Persistable(), _data(ps) 
    { }

private:
    dafBase::PropertySet::Ptr _data;

    int _names(std::list<std::string>& names, bool topLevelOnly=false, 
               bool append=false, int want=3) const;
    int _names(std::vector<std::string>& names, bool topLevelOnly=false, 
               bool append=false, int want=3) const;

    std::vector<dafBase::Persistable::Ptr> 
        _getPersistList(const std::string& name) const 
    {
        POL_GETLIST(name, Persistable::Ptr, FILE)
    }
    std::vector<dafBase::PropertySet::Ptr> 
        _getPropSetList(const std::string& name) const 
    {
        POL_GETLIST(name, dafBase::PropertySet::Ptr, POLICY)
    }

    static Policy *_createPolicy(PolicySource& input, bool doIncludes,
                                 const fs::path& repos, bool validate);
    static Policy *_createPolicy(const std::string& input, bool doIncludes,
                                 const fs::path& repos, bool validate);
};

inline std::ostream& operator<<(std::ostream& os, const Policy& p) {
    p.print(os);
    return os;
}

/*  ************** Inline Class functions ********************  */

inline int Policy::names(std::list<std::string>& names, bool topLevelOnly, 
                         bool append) const
{
    return _names(names, topLevelOnly, append, 7);
}
inline int Policy::paramNames(std::list<std::string>& names, bool topLevelOnly,
                              bool append) const
{
    return _names(names, topLevelOnly, append, 4);
}
inline int Policy::policyNames(std::list<std::string>& names, 
                               bool topLevelOnly,
                               bool append) const
{
    return _names(names, topLevelOnly, append, 1);
}
inline int Policy::fileNames(std::list<std::string>& names, bool topLevelOnly, 
                             bool append) const
{
    return _names(names, topLevelOnly, append, 2);
}

inline Policy::StringArray Policy::names(bool topLevelOnly) const { 
    return _data->names(); 
}
inline Policy::StringArray Policy::paramNames(bool topLevelOnly) const {
    StringArray out;
    _names(out, topLevelOnly, true, 4);
    return out;
}
inline Policy::StringArray Policy::policyNames(bool topLevelOnly) const {
    return _data->propertySetNames(topLevelOnly);
}
inline Policy::StringArray Policy::fileNames(bool topLevelOnly) const {
    StringArray out;
    _names(out, topLevelOnly, true, 2);
    return out;
}

inline size_t Policy::valueCount(const std::string& name) const { 
    return _data->valueCount(name); 
}

inline bool Policy::isArray(const std::string& name) const { 
    return _data->isArray(name); 
}

inline bool Policy::exists(const std::string& name) const { 
    return _data->exists(name); 
}

inline bool Policy::isBool(const std::string& name) const {
    try {
        return (_data->typeOf(name) == typeid(bool));
    }
    catch (...) {
        return false;
    }
}

inline bool Policy::isInt(const std::string& name) const {
    try {
        return (_data->typeOf(name) == typeid(int));
    }
    catch (...) {
        return false;
    }
}

inline bool Policy::isDouble(const std::string& name) const {
    try {
        return (_data->typeOf(name) == typeid(double));
    }
    catch (...) {
        return false;
    }
}

inline bool Policy::isString(const std::string& name) const {
    try {
        return (_data->typeOf(name) == typeid(std::string));
    }
    catch (...) {
        return false;
    }
}

inline bool Policy::isPolicy(const std::string& name) const {
    try {
        return (_data->typeOf(name) == typeid(dafBase::PropertySet::Ptr));
    }
    catch (...) {
        return false;
    }
}

inline bool Policy::isFile(const std::string& name) const {
    
    try {
        return (getValueType(name) == FILE);
    }
    catch (...) {
        return false;
    }
}

inline const std::type_info& Policy::getTypeInfo(const std::string& name) const
{
    try {  return _data->typeOf(name); }
    catch (pexExcept::NotFoundException& e) {
        throw LSST_EXCEPT(NameNotFound, name);
    }
}

inline const std::type_info& Policy::typeOf(const std::string& name) const {
    return getTypeInfo(name); 
}

inline Policy::ConstPtr Policy::getPolicy(const std::string& name) const {
    return ConstPtr(new Policy(_data->get<dafBase::PropertySet::Ptr>(name)));
}
inline Policy::Ptr Policy::getPolicy(const std::string& name) {
    return Ptr(new Policy(_data->get<dafBase::PropertySet::Ptr>(name)));
}

inline 
Policy::StringArray Policy::getStringArray(const std::string& name) const {
    return _data->getArray<std::string>(name);
}

inline Policy::BoolArray Policy::getBoolArray(const std::string& name) const {
    POL_GETLIST(name, bool, BOOL) 
}

inline Policy::IntArray Policy::getIntArray(const std::string& name) const {
    POL_GETLIST(name, int, INT) 
}

inline 
Policy::DoubleArray Policy::getDoubleArray(const std::string& name) const {
    POL_GETLIST(name, double, DOUBLE) 
}

inline void Policy::set(const std::string& name, const Ptr& value) {
    _data->set(name, value->asPropertySet());
}
inline void Policy::set(const std::string& name, bool value) { 
    _data->set(name, value); 
}
inline void Policy::set(const std::string& name, int value) { 
    _data->set(name, value); 
}
inline void Policy::set(const std::string& name, double value) { 
    _data->set(name, value); 
}
inline void Policy::set(const std::string& name, const std::string& value) { 
    _data->set(name, value); 
}
inline void Policy::set(const std::string& name, const char *value) { 
    _data->set(name, std::string(value)); 
}

#define POL_ADD(name, value) \
    try {  _data->add(name, value);  } \
    catch(dafBase::TypeMismatchException&) { \
        throw LSST_EXCEPT(TypeError, name, getTypeName(name));  \
    }

inline void Policy::add(const std::string& name, const Ptr& value) {
    POL_ADD(name, value->asPropertySet())
}
inline void Policy::add(const std::string& name, bool value) { 
    POL_ADD(name, value); 
}
inline void Policy::add(const std::string& name, int value) { 
    POL_ADD(name, value); 
}
inline void Policy::add(const std::string& name, double value) { 
    POL_ADD(name, value); 
}
inline void Policy::add(const std::string& name, const std::string& value) { 
    POL_ADD(name, value); 
}
inline void Policy::add(const std::string& name, const char *value) { 
    POL_ADD(name, std::string(value)); 
}



inline Policy* Policy::createPolicy(PolicySource& input, bool doIncludes, 
                                    bool validate) 
{
    return _createPolicy(input, doIncludes, fs::path(), validate);
} 

inline Policy* Policy::createPolicy(const std::string& input, bool doIncludes, 
                                    bool validate) 
{
    return _createPolicy(input, doIncludes, fs::path(), validate);
} 

inline Policy* Policy::createPolicy(PolicySource& input, 
                                    const fs::path& repository, bool validate) 
{
    return _createPolicy(input, true, repository, validate);
}

inline Policy* Policy::createPolicy(PolicySource& input, 
                                    const std::string& repository, 
                                    bool validate) 
{
    return _createPolicy(input, true, fs::path(repository), validate);
}

inline Policy* Policy::createPolicy(PolicySource& input, 
                                    const char *repository, 
                                    bool validate) 
{
    return _createPolicy(input, true, fs::path(repository), validate);
}

inline Policy* Policy::createPolicy(const std::string& input, 
                                    const fs::path& repository, bool validate) 
{
    return _createPolicy(input, true, repository, validate);
}

inline Policy* Policy::createPolicy(const std::string& input, 
                                    const std::string& repository, 
                                    bool validate) 
{
    return _createPolicy(input, true, fs::path(repository), validate);
}

inline Policy* Policy::createPolicy(const std::string& input, 
                                    const char *repository, 
                                    bool validate) 
{
    return _createPolicy(input, true, fs::path(repository), validate);
}

inline dafBase::PropertySet::Ptr Policy::asPropertySet() { return _data; }

// general case is disallowed; known types are specialized
template <class T> T Policy::getValue(const std::string& name) const {
    throw LSST_EXCEPT(TypeError, name, std::string("not implemented for this type"));
}

template <class T> std::vector<T> Policy::getValueArray(const std::string& name) const {
    throw LSST_EXCEPT(TypeError, name, std::string("not implemented for this type"));
}

template <class T> Policy::ValueType Policy::getValueType() {
    throw LSST_EXCEPT(TypeError, std::string("unknown"), std::string("not implemented for this type"));
}

}}}  // end namespace lsst::pex::policy

#endif // LSST_PEX_POLICY_POLICY_H
