//	Astrophysics Science Division,
//	NASA/ Goddard Space Flight Center
//	HEASARC
//	http://heasarc.gsfc.nasa.gov
//	e-mail: ccfits@legacy.gsfc.nasa.gov
//
//	Original author: Ben Dorman

// Table
#include "Table.h"
// PHDU
#include "PHDU.h"
// PrimaryHDU
#include "PrimaryHDU.h"
// FITS
#include "FITS.h"

#ifdef _MSC_VER
#include "MSconfig.h" // for truncation warning
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef SSTREAM_DEFECT
#include <strstream>
#else
#include <sstream>
#endif

using std::endl;
using std::cout;
using std::cerr;

#include <memory>
#include <iterator>


namespace CCfits {
  
  typedef  std::multimap<string,ExtHDU*> ExtMap;
  typedef  std::multimap<string,ExtHDU*>::iterator ExtMapIt;
  typedef  std::multimap<string,ExtHDU*>::const_iterator ExtMapConstIt;

  
  // Class CCfits::FITS::NoSuchHDU 

  FITS::NoSuchHDU::NoSuchHDU (const String& diag, bool silent)
  : FitsException("FITS Error: Cannot read HDU in FITS file: ",silent)
  {
  //! Exception to be thrown by failed seek operations
     addToMessage(diag);
  if ( FITS::verboseMode() || !silent) std::cerr << diag << "\n";
  }


  // Class CCfits::FITS::OperationNotSupported 

  FITS::OperationNotSupported::OperationNotSupported (const String& msg, bool silent)
  : FitsException("FITS Error: Operation not supported: ",silent)
  {
    addToMessage(msg);
    if ( FITS::verboseMode() || !silent)  std::cerr << msg << "\n";
  }


  // Class CCfits::FITS::CantOpen 

  FITS::CantOpen::CantOpen (const String& diag, bool silent)
   : FitsException("FITS Error: Cannot open file ",silent)
  {
    addToMessage(diag);
    if ( FITS::verboseMode() || !silent)  std::cerr << diag << "\n";
  }


  // Class CCfits::FITS::CantCreate 

  FITS::CantCreate::CantCreate (const String& diag, bool silent)
   : FitsException(string("FITS Error: Cannot create file "),silent)
  {
     addToMessage(diag);
  if ( FITS::verboseMode() || !silent) std::cerr << diag << '\n';
  }


  // Class CCfits::FITS 
  bool FITS::s_verboseMode = false;

  FITS::FITS (const String &fileName, RWmode rwmode, bool readDataFlag, const std::vector<String>& primaryKeys)
  : m_currentCompressionTileDim(0),
    m_mode(rwmode), m_currentExtensionName(""), m_filename(fileName),
    m_pHDU(0), m_extension(), m_fptr(0)
  {
    if (rwmode == Read) 
    {
      // If name includes extended file syntax, the initial hdu position
      // upon opening is not necessarily the primary header.
      int hduIdx = open(rwmode);

      // Read the primary header.  This also moves the current hdu
      // to the primary.
      read(readDataFlag,primaryKeys);

      readExtensions(readDataFlag);

      // If extended syntax asked for a particular extension,
      // restore it as the current hdu position.
      if (hduIdx)
      {
         // This also calls makeThisCurrent.
         extension(hduIdx);
      }
    }
    else
    {

      // create a primary header in the receiving structure. 
      // PHDU has a private constructor and must be instantiated here only.
      // this ensures that every FITS object has exactly one PHDU.
      if (create() )
      {
         // create returns true if the file is either new
         // or overwritten, in which case we need to create 
         // and write a new primary with BITPIX=8 and NAXIS=0.
         // Also if in here, no extended syntax was used.
         HDUCreator makePrimary(this);

         pHDU(makePrimary.createImage(8,0,std::vector<long>()));       
      }
      else
      {
         // The create call above will have opened the file in rw mode.  
         read(readDataFlag,primaryKeys);
         readExtensions(readDataFlag);
         // For backwards compatibility, reposition the current HDU to
         // be the primary.  (In earlier versions, only the primary was
         // ever read when opening a pre-existing file in Write mode.)
         resetPosition();
      }
    }
  }

  FITS::FITS (const String &fileName, RWmode rwmode, const string &hduName, bool readDataFlag, const std::vector<String>& hduKeys, const std::vector<String>& primaryKey, int version)
  : m_currentCompressionTileDim(0),
    m_mode(rwmode), m_currentExtensionName(""), m_filename(fileName),
    m_pHDU(0), m_extension(), m_fptr(0)
  {
    int extSyntHdu = open(rwmode);

    // create and read the Primary: assume here that a single HDU other than
    // the primary is requested, so don't read primary data. We can however
    // read some header info from the primary if (optionally) specified.

    read(false,primaryKey);

    read(hduName, readDataFlag, hduKeys,version);
    if (extSyntHdu && currentExtension().index() != extSyntHdu)
    {
      std::ostringstream msg;
      msg << "Hdu (" << hduName << ") requested with extended syntax ("
          << extSyntHdu << ") differs from that requested by name argument ("
          << currentExtension().index() << ").";
      throw OperationNotSupported(msg.str());
    }
  }

  FITS::FITS (const String &fileName, RWmode rwmode, const std::vector<String>& hduNames, bool readDataFlag, const std::vector<String>& primaryKey)
  : m_currentCompressionTileDim(0),
    m_mode(rwmode), m_currentExtensionName(""), m_filename(fileName),
    m_pHDU(0), m_extension(), m_fptr(0)
  {
    int extSyntHdu = open(rwmode);

    read(readDataFlag,primaryKey);
    read(hduNames,readDataFlag);

    if (extSyntHdu)
    {
      // If hdu specified in extension is not included in
      // hduNames, throw.
      bool savVerbose = s_verboseMode;
      s_verboseMode = false;
      try
      {
         extension(extSyntHdu);
      }
      catch (...)
      {
         s_verboseMode = savVerbose;
         string msg("Hdu requested with extended syntax was not included ");
         msg += "in FITS constructor hduNames array.";
         throw OperationNotSupported(msg);
      }
      s_verboseMode = savVerbose;
    }
  }

  FITS::FITS (const String& fileName, const FITS& source)
  : m_currentCompressionTileDim(0),
    m_mode(Write), m_currentExtensionName(""), m_filename(fileName),
    m_pHDU(0), m_extension(), m_fptr(0)
  {
    if (create() )
    {
      // create returns true if the file is either new
      // or overwritten, in which case we need to create 
      // and write a new primary which is a clone of the source PHDU.

      pHDU(static_cast<PHDU*>(source.pHDU().clone(this)));       
    }
    else
    {
      // Assume file already exists and user is not attempting to
      // overwrite with the '!' symbol.
      throw CantCreate(fileName);
    }    
    int status(0);

    source.pHDU().makeThisCurrent();

    if (fits_copy_hdu(source.fitsPointer(),m_fptr,0,&status)) throw FitsError(status);
  }

  FITS::FITS (const String &fileName, RWmode rwmode, const std::vector<String>& hduNames, const std::vector<std::vector<String> >& hduKeys, bool readDataFlag, const std::vector<String>& primaryKeys, const std::vector<int>& hduVersions)
  : m_currentCompressionTileDim(0),
    m_mode(rwmode), m_currentExtensionName(""), m_filename(fileName),
    m_pHDU(0), m_extension(), m_fptr(0)
  {
    int extSyntHdu = open(rwmode);

    // read the primary header and read the data if readDataFlag is set.
    read(readDataFlag,primaryKeys);

    read(hduNames, hduKeys, readDataFlag, hduVersions);

    if (extSyntHdu)
    {
      // If hdu specified in extension is not included in
      // hduNames, throw.
      bool savVerbose = s_verboseMode;
      s_verboseMode = false;
      try
      {
        extension(extSyntHdu);
      }
      catch (...)
      {
        s_verboseMode = savVerbose;
        string msg("Hdu requested with extended syntax was not included ");
        msg += "in FITS constructor hduNames array.";
        throw OperationNotSupported(msg);
      }
      s_verboseMode = savVerbose;
    }
  }

  FITS::FITS (const String& fileName, int bitpix, int naxis, long *naxes)
  : m_currentCompressionTileDim(0),
    m_mode(Write), m_currentExtensionName(""), m_filename(fileName),
    m_pHDU(0), m_extension(), m_fptr(0)
  {
    std::vector<long>    va_naxes(naxis);
    std::copy(&naxes[0],&naxes[naxis],va_naxes.begin());

    if (!create())
    {
      // Assume file already exists and user did not specify overwrite
      // with the '!' symbol.
      throw CantCreate(fileName);
    }

    // create an HDU factory.
    HDUCreator makePrimary(this);

    // set the PrimaryHDU. bitpix will determine the data type.
    pHDU(makePrimary.createImage(bitpix,naxis,va_naxes));

    // If file name is explicitly indicating image compression,
    // pHDU won't hold the image.  Therefore need to create an
    // extension here.
    string::size_type compressLoc = 
            FITSUtil::checkForCompressString(name());
    if (compressLoc != string::npos)
    {
      HDUCreator newImage(this);
      ExtHDU* newHDU = newImage.createImage(string("NoName"),bitpix, naxis, va_naxes, 1);  
      addExtension(newHDU);
      string actualFileName(name().substr(0, compressLoc));
      m_filename = actualFileName;
      currentCompressionTileDim(naxis);
    }
  }

  FITS::FITS (const string &fileName, RWmode rwmode, int hduIndex, bool readDataFlag, const std::vector<String>& hduKeys, const std::vector<String>& primaryKey)
  : m_currentCompressionTileDim(0),
    m_mode(rwmode), m_currentExtensionName(""), m_filename(fileName),
    m_pHDU(0), m_extension(), m_fptr(0)
  {
    int extSyntHdu = open(rwmode);
    if (extSyntHdu && extSyntHdu != hduIndex)
    {
      string msg("FITS constructor hduIndex conflicts with HDU requested by extended syntax.");
      throw OperationNotSupported(msg);
    }
    // read the primary header. Allowing the user to read the primary data
    // and optional keys costs nothing here, although the likely use of this
    // constructor is to read a specified HDU from a file other than the Primary.
    read(readDataFlag,primaryKey);

    read(hduIndex,readDataFlag,hduKeys);
  }

  FITS::FITS (const String &fileName, RWmode rwmode, const std::vector<String>& searchKeys, const std::vector<String> &searchValues, bool readDataFlag, const std::vector<String>& hduKeys, const std::vector<String>& primaryKey, int version)
  : m_currentCompressionTileDim(0),
    m_mode(rwmode), m_currentExtensionName(""), m_filename(fileName),
    m_pHDU(0), m_extension(), m_fptr(0)
  {
    open(rwmode);

    // read the primary header and read the data if readDataFlag is set.
    // create returns a PHDU if the index is 0.
    read(false,primaryKey);

    // create a primary header in the receiving structure.
    // PHDU has a private constructor and must be instantiated here only.
    // this ensures that every FITS object has exactly one PHDU
    read(searchKeys, searchValues, readDataFlag, hduKeys, version);
  }

  FITS::~FITS()
  {
    destroy();      
  }
  
  void FITS::unmapExtension (ExtHDU& doomed)
  {
    const string& doomedName = doomed.name();
    if ( m_extension.count(doomedName) == 1 )
    {
      ExtMapIt x = m_extension.lower_bound(doomedName);
      delete (*x).second;       
      m_extension.erase(x);
    }
    else
    {
      std::pair<ExtMapIt,ExtMapIt> named = m_extension.equal_range(doomedName);

      ExtMapIt x = named.first;

      while ( x != named.second)
      {

        if ( (*x).second->version() == doomed.version())
        {
          delete (*x).second;
          m_extension.erase(x);       
          break;
        }       
        ++x;
      }
    }   
  }

  void FITS::clearErrors ()
  {
    fits_clear_errmsg();
  }

  void FITS::deleteExtension (const String& doomed, int version)
  {
    int status(0);
    ExtHDU& d = extension(doomed,version); // throws NoSuchHDU if it's not there.
    const int removeIdx = d.index();
    std::vector<ExtHDU*> trailingExts;
    ExtMapConstIt itExtMap = m_extension.begin();
    ExtMapConstIt itExtMapEnd = m_extension.end();
    while (itExtMap != itExtMapEnd)
    {
       if (itExtMap->second->index() > removeIdx)
          trailingExts.push_back(itExtMap->second);
       ++itExtMap;
    }

    if (fits_delete_hdu(m_fptr,0,&status)) throw FitsError(status);
    unmapExtension(d);
    // Reindex the extensions that follow the deleted.
    for (size_t i=0; i<trailingExts.size(); ++i)
       trailingExts[i]->index(trailingExts[i]->index()-1);
  }

  int FITS::nextVersionNumber (const String& inputName) const
  {
    int n(0);
    int status(0);
    int current(0);
    if (fits_get_num_hdus(m_fptr,&n,&status)) throw FitsError(status);
    fits_get_hdu_num(m_fptr,&current);

    int count(0);
    for (int j = 2; j <= n; ++j)
    {
      if (nameOfUnmapped(j) == inputName) ++count;
    }
    if (fits_movabs_hdu(m_fptr,current,0,&status)) throw FitsError(status);
    return count+1;      
  }

  void FITS::read (bool readDataFlag, const std::vector<String>& keys)
  {
     // Move to and create primary header object.
   HDUCreator create(this);

   int status=0, hduType=0;
   if (fits_movabs_hdu(m_fptr, 1, &hduType, &status))
      throw FitsError(status);
   pHDU(static_cast<PHDU*>(create.getHdu(0,readDataFlag,keys)));
  }

  void FITS::read (const String &hduName, bool readDataFlag, const std::vector<String> &keys, int version)
  {
    // grab the requested extension if present. If not, create an ExtHDU object and add it, if it
    // exists in the file. The first clause copes with the results from the ctor that reads the 
    // entire file for HDUs, the second when adding a new HDU from a file which selected HDUs were
    // read on construction.
    ExtHDU* requested = checkAlreadyRead(0, hduName, version);

    if (!requested)
    {
       HDUCreator create(this);

       try
       {
          // primary is always false here.   
          ExtHDU* newHDU = static_cast<ExtHDU*>(create.getHdu(hduName, readDataFlag, keys, false, version));
          // add a specified HDU in the file to the list of extensions.
          addExtension(newHDU);
       }
       catch ( ... )
       {
          std::ostringstream msg;
          msg << hduName << " with version " << version;
          throw NoSuchHDU(msg.str());
       }
    }
    else
    {
       requested->makeThisCurrent();
       requested->readData(readDataFlag,keys);
    }
  }

  void FITS::read (const std::vector<String> &hduNames, bool readDataFlag)
  {
    const size_t nHdu = hduNames.size();
    const std::vector<String> dummy;
    for (size_t i=0; i<nHdu; ++i)
    {
       try
       {
          read(hduNames[i], readDataFlag, dummy, 1);
       }
       catch (NoSuchHDU&)
       {
          // If one Hdu is missing, keep trying the rest.
          continue;
       }
    }
  }

  void FITS::read (const std::vector<String> &hduNames, const std::vector<std::vector<String> > &keys, bool readDataFlag, const std::vector<int>& hduVersions)
  {
    const size_t nHdu = hduNames.size();
    const size_t nKeys = keys.size();
    const size_t nVersions = hduVersions.size();
    const std::vector<String> dummy;

    for (size_t i=0; i<nHdu; ++i)
    {
       const std::vector<String>& hduKeys = (i < nKeys) ? keys[i] : dummy;
       int version = (i < nVersions) ? hduVersions[i] : 1;
       try
       {
          read(hduNames[i], readDataFlag, hduKeys, version);
       }
       catch (NoSuchHDU&)
       {
          continue;
       }
    }
  }

  void FITS::read (int hduIndex, bool readDataFlag, const std::vector<String> &keys)
  {
    if (hduIndex == 0)
    {
       std::cerr << "Primary header is always read, doesn't need to be requested."
          << std::endl;
       return;
    }

    ExtHDU* requested = checkAlreadyRead(hduIndex);

    if (!requested)
    {
       HDUCreator create(this);

       try
       {
          ExtHDU* newHDU = static_cast<ExtHDU*>(create.getHdu(hduIndex,readDataFlag,keys));
          // add a specified HDU in the file to the list of extensions.
          addExtension(newHDU);
       }
       catch ( ... )
       {
          std::ostringstream msg;
          msg << "Error: Could not read extension: " << hduIndex;
          throw NoSuchHDU(msg.str());
       }
    }
    else
    {
       requested->makeThisCurrent();
       requested->readData(readDataFlag,keys);
    }
  }

  void FITS::read (const std::vector<String>& searchKeys, const std::vector<String> &searchValues, bool readDataFlag, const std::vector<String>& hduKeys, int version)
  {
  int totalHDUs = 1;
  int status = 0;
  if (fits_get_num_hdus(m_fptr,&totalHDUs,&status) != 0) throw FitsError(status);
  HDUCreator createTest(this);

  int hduIndex;
  // there's one less extension than total HDUs!
  bool gotIt = false;
  for  (hduIndex = 1; hduIndex < totalHDUs; hduIndex++ )
  {
     try
     {
        std::unique_ptr<ExtHDU> 
	   testHDU(static_cast<ExtHDU*>(createTest.getHdu(hduIndex,false,searchKeys)));
	std::vector<String> testKeys(searchKeys);
	std::vector<String> testResults(searchKeys.size(),"");
        // Missing key exceptions are caught and handled at a lower level.
        // The result will be a smaller sized testKeys vector.
	testHDU->readKeys(testKeys,testResults); 
        using namespace FITSUtil;

	// first: we need to have matched as many keys as were input.

	if (testKeys.size() == searchKeys.size()) 
	{
	   // now go through and check the values that were read
	   // against the input value list.
	   size_t k = 0;
	   gotIt = true;   
           std::vector<String>::const_iterator vi(searchValues.begin());   
          std::vector<String>::const_iterator  viEnd(searchValues.end());   

           while (vi != viEnd && gotIt)
	   {

	      if (vi->length())
	      {
                   // we can at least ignore whitespace
                   size_t first (testResults[k].find_first_not_of(" \t"));
                   size_t last  (testResults[k].find_last_not_of(" \t"));
		   gotIt = (lowerCase(testResults[k].substr(first,last+first+1)) == lowerCase(*vi));
	      }
              ++k,++vi;
	   }

	   if (gotIt)
	   {
	      if (version == 1) break;
	      else
	      {
		 int extver = 1;
#ifdef TEMPLATE_AMBIG_DEFECT
		 testHDU->readKeyMS("EXTVER",extver);
#else
		 testHDU->readKey("EXTVER",extver);                                        
#endif
		 if (extver == version) break;
		 else gotIt = false;
	      }
	   }
	}
     }

     catch (HDU::NoSuchKeyword)
     {
        // catches the case where the version is requested but there
        // is no version key present.
        createTest.reset();
        continue;
     }

     catch (FitsError)
     {                
        createTest.reset();
        continue;       
     }

     catch (...)
     {
        throw;
     }

     createTest.reset();        
  }

  if (gotIt)
  {
     String extname("");
     int extver = 1;
     ExtHDU::readHduName(m_fptr,hduIndex,extname,extver);
     read(extname,readDataFlag,hduKeys,extver);
     currentExtensionName(extname);
  }

  else
  {
#ifdef SSTREAM_DEFECT
     std::ostrstream err;
#else
     std::ostringstream err;
#endif
      err << "Error: Unable to find extension containing keywords ";
      std::copy(searchKeys.begin(),searchKeys.end(),
		std::ostream_iterator<String>(err," "));
      err << "\nwith required values and version number ";	
#ifdef SSTREAM_DEFECT
      err << std::ends;
#endif

      throw NoSuchHDU(err.str());
  }     
  }

  int FITS::open (RWmode rwmode)
  {
  int status=0;
  // unused:  bool silentMode = true;

  status = fits_open_file(&m_fptr, name().c_str(), rwmode, &status);

  if (status != 0)  
  {
          if (status == FILE_NOT_OPENED) throw CantOpen(name()); 
          else throw FitsError(status);
  }

  int currentHDU=0;
  fits_get_hdu_num(m_fptr, &currentHDU);
  // convert to 0-based
  currentHDU -= 1;

  return currentHDU;
  }

  bool FITS::create ()
  {

     int status(0);

     // if the filename indicates overwrite (starts with a '!' - see 
     // fits_create_file documentation)
     // or the filename does not correspond to an existing file, create file.
     // otherwise throw an overwrite exception.

     // if the file is writeable and the name doesn't begin with '!'
     // we want read/write access so should use fits_open_file,
     // otherwise create it. We let cfitsio worry about what
     // happens if open-with-write-mode or create fails.
     // but we must know whether the file exists.
     string fName = m_filename;
     if (fName[0] == '!') 
     {
        m_filename = fName.substr(1);
     }

     // Create a new file, the '!' is processed by ffinit.
     // If '[]' extended syntax is used this returns an error of
     // one kind or another (which includes FILE_NOT_CREATED if
     // the file already exists).
     fits_create_file(&m_fptr,fName.c_str(),&status);
     if ( status )
     {
        // The open function must succeed, else we're left with
        // a NULL m_fptr with which we can't continue.
        try
        {
           if (status == FILE_NOT_CREATED ) 
           {
              string warn(" File already exists: ");
              warn += fName;
              warn += " attempting to open existing file";
              if (verboseMode()) std::cerr << warn << '\n';
              open(Write);
           }
           else
           {
              throw CantCreate(fName);
           }
        }
        catch (FitsException&) { throw; } 
        return false;
     }
     return true; 
  }

  int FITS::close () throw ()
  {
    int status=0;

    status = fits_close_file(m_fptr, &status);
    if (!status) m_fptr = 0;
    return status;
  }

  const ExtHDU& FITS::extension (int i) const
  {

  const ExtMap& ext = m_extension;
  ExtMapConstIt hduByNum = ext.begin();
  ExtMapConstIt endOfList = ext.end();
  while (hduByNum != endOfList)
  {
          if ((*hduByNum).second->index() == i) break;
          ++hduByNum;
  }

  if (hduByNum == endOfList) 
  {
#ifdef SSTREAM_DEFECT
       std::strstream msg;
#else
       std::ostringstream msg;
#endif
       msg << "No HDU with index " << i << '\n';
       throw NoSuchHDU(msg.str());
  }
  (*hduByNum).second->makeThisCurrent();
  return *((*hduByNum).second);   
  }

  std::ostream & FITS::put (std::ostream &s) const
  {
  s << "FITS:: Primary HDU: \n" ;

  s << pHDU() << endl;

  s << "FITS:: Extensions: \n";

  const ExtMap& ext = m_extension;
  ExtMapConstIt endOfList = ext.end();

  for (ExtMapConstIt it = ext.begin();
       it != endOfList;
       ++it)
     s << *((*it).second) << endl;

  return s;
  }

  ExtHDU& FITS::extension (int i)
  {
  ExtMap& ext = m_extension;
  ExtMapIt hduByNum = ext.begin();
  ExtMapIt endOfList = ext.end();

  while (hduByNum != endOfList)
  {
     if ((*hduByNum).second->index() == i)  break;
     ++hduByNum;     
  }

  if (hduByNum == endOfList) 
  {
#ifdef SSTREAM_DEFECT
       std:: strstream msg;
#else
       std::ostringstream msg;
#endif
       msg << "No HDU with index " << i;
       throw NoSuchHDU(msg.str());
  }
  (*hduByNum).second->makeThisCurrent();
  return *((*hduByNum).second);    
  }

  const ExtHDU& FITS::extension (const String& hduName, int version) const
  {
  return extbyVersion(hduName,version);
  }

  ExtHDU& FITS::extbyVersion (const String& hduName, int version) const
  {
  // hey! remember it's a multimap and we need to work a little harder!
  // first, for convenience...
  // how many extensions with name hduName?  
  const ExtMap& ext = m_extension;        
  ExtMap::size_type n = ext.count(hduName);

  if (n == 0) 
  {
#ifdef SSTREAM_DEFECT
       std::strstream msg;
#else
       std::ostringstream msg;
#endif
       msg << "No HDU with name " << hduName;
       if (version) msg << " and version " << version;
       throw NoSuchHDU(msg.str());
  }

  // ignore version checking if there is only one version present.
        ExtMapConstIt c = ext.lower_bound(hduName);
        if ( n > 1)
        {
             ExtMapConstIt last = ext.upper_bound(hduName);
             while ( c != last )
             {
                    if ((*c).second->version() == version) break;
                    c++;
             }
             if ( c == last )
             {
#ifdef SSTREAM_DEFECT
                 std::ostrstream msg;
#else
                 std::ostringstream msg;
#endif
                 msg << "No HDU with name " << hduName;
                 if (version != 1) msg << " and version " << version;
#ifdef SSTREAM_DEFECT
	         msg << std::ends;
#endif         
                 throw NoSuchHDU(msg.str());
             }
        }
        (*c).second->makeThisCurrent();
        return *((*c).second);
  }

  void FITS::readExtensions (bool readDataFlag)
  {
   HDUCreator create(this);
   int status = 0;
   int numHDUs = 0;

   if (fits_get_num_hdus(m_fptr,&numHDUs,&status) != 0) throw FitsError(status);

   // Not clearly exception safe : revisit!!!

   // Unused:   ExtHDU* newExt = 0;
   // there's one less Extension than HDUs. Here, if there are 5 HDUs there are 4 extensions,
   // and this loop should execute 4 times. HDU index 1 is the first extension, not the primary.
   for (int i = 1; i < numHDUs ; i++)
   {
       addExtension(static_cast<ExtHDU*>(create.getHdu(i,readDataFlag)));
       create.reset();
   }
  }

  Table* FITS::addTable (const String& hduName, int rows, 
          const std::vector<String>& columnName, 
          const std::vector<String>& columnFmt, 
          const std::vector<String>& columnUnit, 
          HduType type, int version)
  {
   ExtHDU* current(0);
   size_t N(m_extension.count(hduName));
   std::pair<ExtMapIt,ExtMapIt> matches(m_extension.equal_range(hduName));
   if ( N > 0 )
   {
      ExtMapIt s(matches.first);
      while  ( s != matches.second )
      {
         if (s->second->version() == version && dynamic_cast<Table*>(s->second) )
         {
            current = s->second;
            std::cerr << " Table Extension " << hduName << " with version " 
                    << version << " already exists "
                    << " returning token for existing version \n";
         }
         ++s;       
      } 
   }      
   if ( !current )
   {
      HDUCreator newTable(this);
      Table* newHDU = static_cast<Table*>(newTable.createTable(hduName, type, rows,
                                               columnName, columnFmt,  columnUnit, version));   
      current = addExtension(newHDU);
   }
   return static_cast<Table*>(current);
  }
  
  Table * FITS::addGroupTable(const String & groupName, int groupID)
  {
    // +++ if I wanted to call addTable and specify HduType type = GroupTbl, I would need to provide column defs
    //     plus, GroupTbl isn't a real HduType according to cfitsio, so I don't know if I want to add it to that enum
    //     so I will mimic a lot of addTable.  I'm not sure if all this repeating is correct.
    
    string hduName("GROUPING");
    ExtHDU* current(0);
    size_t N(m_extension.count(hduName));
    std::pair<ExtMapIt,ExtMapIt> matches(m_extension.equal_range(hduName));
    if ( N > 0 )
    {
       ExtMapIt s(matches.first);
       while  ( s != matches.second )
       {
          // +++ I think version means EXTVER
          if (s->second->version() == groupID && dynamic_cast<Table*>(s->second) )
          {
             current = s->second;
             std::cerr << " Table Extension " << hduName << " with version " 
                     << groupID << " already exists "
                     << " returning token for existing version \n";
          }
          ++s;     
       }
    }      
    if ( !current )
    {
      HDUCreator newTable(this);
      ExtHDU* newHDU = static_cast<Table*>(newTable.createTable(groupName, GroupTbl, 0, std::vector<String>(), std::vector<String>(),  std::vector<String>(), groupID));   
      current = addExtension(newHDU);
    }
    
    // now assign the grouping name
    // +++ but HDU::addKeyword is private?
    
    return static_cast<Table*>(current);
    //return static_cast<GroupTable*>(current);


    
    
    
    
    
//    
//    // the C prototypes don't use const, so these casts are necessary.       
//    char * gName = const_cast<char*>(groupName.c_str());  
//    
//    // +++ another option is to call addTable with the columns we want, and not use the built in cfitsio fn
//    // +++ just add it to the end of the file for now
//    if ( fits_create_group(m_fptr, gName, GT_ID_ALL_URI, &status) ) throw FitsError(status);
//    
//    // which extension index is the new ext
//    int numHDUs = fits_get_num_hdus (m_fptr, > int *hdunum, &status);
//    if (status) throw FitsError(status);
//    
//    HDUCreator newTable(this);
//    // index of primary = 0
//    HDU * newHDU = newTable.getHdu(numHDUs-1);
//    
//    // create the GroupTable
//    GroupTable * groupTable;
//    
//    
//    // now add the GroupTable to this FITS file
//    // I know that this latest HDU is an extension, since I just added it,
//    // so I can safely cast
//    ExtHDU * newExtHDU = addExtension(static_cast<ExtHDU*>());
//    //+++ if (newExtHDU == 0) throw ;
//    
    
    
  }

  ExtHDU* FITS::addImage (const String& hduName, int bpix, std::vector<long>& naxes, int version)
  {
    ExtHDU* current(0);
    size_t N(m_extension.count(hduName));
    std::pair<ExtMapIt,ExtMapIt> matches(m_extension.equal_range(hduName));
    if ( N > 0 )
    {
       ExtMapIt s(matches.first);
       while  (!current && s != matches.second )
       {
          if ( s->second->version() == version  )
          {
             std::cerr << " Extension " << hduName << " with version " 
                     << version << " already exists "
                     << " returning token for existing version \n";
             current = s->second;

          }
          ++s;       
       } 
    }      
    if (!current)
    {
         HDUCreator newImage(this);
         ExtHDU* newHDU = newImage.createImage(hduName, bpix, naxes.size(), naxes,  version);  
         current =  addExtension(newHDU);
         if (getCompressionType())
         {
            if (static_cast<long>(naxes.size()) > currentCompressionTileDim())
            {
               currentCompressionTileDim(naxes.size());
            }
         }
    }
    return current;
  }

  void FITS::destroy () throw ()
  {
    // close the file associated with this FITS object if it is open.
    // close () can't throw because it only calls cfitsio's close function,
    // so error messages are handled here.
    close();

    // destroyPrimary and destroyExtensions call delete which is guaranteed nothrow,
    // destroyExtensions also calls multimap::clear
    destroyPrimary();    
    destroyExtensions();
    
    m_filename = "";

    // nullify the fitsfile pointer, so that it passes "deallocated" tests
    // such as: if ( x.m_fptr == 0 )
    m_fptr = 0;
  }
  
  
  void FITS::destroyPrimary ()
  {
    delete m_pHDU;
    m_pHDU = 0;
  }

  void FITS::destroyExtensions ()
  {
    ExtMapIt endList = m_extension.end();

    for (ExtMapIt hdu = m_extension.begin(); hdu != endList; hdu++)
    {
      delete (*hdu).second;
    }

    m_extension.clear();
  }
  
  void FITS::flush ()
  {
    int status (0);
    if (fits_flush_file(m_fptr,&status)) throw FitsError(status);
  }

  ExtHDU* FITS::addExtension (ExtHDU* ext)
  {
   int status(0);
   const String& extName = ext->name();
   ExtMap::value_type addHDUEntry(extName,ext);
   currentExtensionName(extName);
   ExtMapIt added = m_extension.insert(addHDUEntry);
   if (fits_set_hdustruc(m_fptr,&status)) throw FitsError(status);
   (*added).second->index(m_fptr->HDUposition);
   return (*added).second;
  }

  void FITS::resetPosition ()
  {
    int status(0);
    if (fits_movabs_hdu(m_fptr,1,0,&status)) throw FitsError(status);
    currentExtensionName(string(""));
  }
  
  void FITS::copy (const HDU& source)
  {
    int status(0);
    bool isOK = true;    

    const ExtHDU* testSource = dynamic_cast<const ExtHDU*>(&source);
    if (!testSource)
    {
       // If not an ExtHDU, source must be a PHDU.  At the moment there's
       // no way to convert it to an ExtHDU, which is necessary to insert
       // it into the extension map. 
       std::cerr << "Cannot copy a primary HDU of one file into an extension of another.\n"; 
       return;
    }

    HDU* hduCopy = source.clone(this);
    std::unique_ptr<ExtHDU> extCopy(static_cast<ExtHDU*>(hduCopy));
    const String& hduName = extCopy->name(); 
    size_t N = m_extension.count(hduName);
    std::pair<ExtMapIt,ExtMapIt> matches(m_extension.equal_range(hduName));
    if ( N > 0 )
    {
       ExtMapIt s(matches.first);
       while  ( s != matches.second )
       {
          ExtHDU* current = s->second;
          if (current->version() == extCopy->version()) 
          {
             std::cerr << " Extension " << hduName << " with version " 
                     << extCopy->version() << " already exists.\n ";
             isOK = false;
             break;
          }
          ++s;       
       } 
    }      

    if (isOK)
    {    
       source.makeThisCurrent();

       if (fits_copy_hdu(source.fitsPointer(),m_fptr,0,&status))
                   throw FitsError(status); 

       // the fits file has been updated, although the data have not 
       // been read.

       addExtension(extCopy.get());
       Table* tableCopy = dynamic_cast<Table*>(extCopy.get());
       extCopy.release();
       if (!tableCopy)
       {
          // Assume this is copying an image
          if (getCompressionType())
          {
             if (extCopy->axes() > currentCompressionTileDim())
             {
                currentCompressionTileDim(extCopy->axes());
             }
          }
       }
    }
  }

  Table& FITS::filter (const String& expression, ExtHDU& inputTable, bool overwrite, bool readData)
  {

    // there are three cases to deal with:
    // a) inputHDU belongs to a different file than *this  
    //    In this case the selectRows call will simply write to this.
    // b) inputHDU belongs to *this and
    //     i) outHDUVersion is different from any version key in inputHDU.
    //     ii) outHDUVersion matches the version key in inputHDU or neither
    //         inputHDU has a version key nor outHDUVersion is specified.

    // In case i) a new HDU with new version is written.
    // In case ii) the instruction is to overwrite inputHDU with the filtered version.
    // in this case the HDU is marked as "unread" by deleting it from the list of
    // HDUs in *this.
       static char EXTVER[] = "EXTVER";


       try
       {
               Table& table = dynamic_cast<Table&>(inputTable);
               fitsfile* inputFitsPointer = inputTable.fitsPointer() ;
               FITSUtil::auto_array_ptr<char> pName1(new char[FLEN_FILENAME]);
               FITSUtil::auto_array_ptr<char> pName2(new char[FLEN_FILENAME]);
               String tableName = table.name();
               int tableVersion(table.version());
               char* cExpression = const_cast<char*>(expression.c_str());
               int status(0);


               // check if the two file names are the same. Checking the pointers
               // are the same doesn't allow for the possibility that there are
               // two views of the same file.
               if (fits_file_name(inputFitsPointer,pName1.get(),&status)) throw FitsError(status);
               if (fits_file_name(m_fptr,pName2.get(),&status)) throw FitsError(status);
               const String& infile = String(pName1.get());
               const String& outfile = String(pName2.get());

               // store list of what has been read so far so it can be read back.
               std::vector<String> 
                    keyStore(table.keyWord().size() + table.column().size());

               std::map<String,Keyword*>::const_iterator keyIt(table.keyWord().begin());
               std::map<String,Keyword*>::const_iterator keyItEnd(table.keyWord().end());               
               ColMap::const_iterator colIt(table.column().begin());
               ColMap::const_iterator colItEnd(table.column().end());

               size_t count(0);
               if (!table.keyWord().empty())
               {
                        while ( keyIt != keyItEnd )
                        {
                                keyStore[count] = (*keyIt).first;       
                                ++keyIt, ++count;
                        }
               }

               if ( !table.column().empty())
               {
                        while ( colIt != colItEnd )
                        {

                                keyStore[count] = (*colIt).first;       
                                ++colIt, ++count;
                        }
               }
               int revisedVersion(tableVersion);
               if (infile ==  outfile)
               {
                       if (overwrite)
                       {
                                // delete the current HDU from the set of "mapped"
                                // since the contents are being changed. Its header
                                // will be remapped afterward, and if readData is
                                // set to true then the resulting filtered data is
                                // also read back.
                                unmapExtension(inputTable);
                       }
                       else
                       {
                                revisedVersion = nextVersionNumber(tableName);   
                                // create a new header from the source and set it to be the
                                // current HDU
                                cloneHeader(inputTable);
                                if (fits_write_key_lng(m_fptr,EXTVER,revisedVersion,0,&status)) 
                                        throw FitsError(status);
                       }

                       if (fits_select_rows(inputFitsPointer,m_fptr,cExpression,&status))
                            throw FitsError(status);
                       if (fits_flush_file(m_fptr,&status)) throw FitsError(status);

               }
               else
               {
                       // create a new header from the source and set it to be the
                       // current HDU
                       revisedVersion = nextVersionNumber(tableName);
                       cloneHeader(inputTable);

                       if (fits_select_rows(inputFitsPointer,m_fptr,cExpression,&status))
                            throw FitsError(status);       

                       if (fits_write_key_lng(m_fptr,EXTVER,revisedVersion,0,&status)) 
                               throw FitsError(status);
                       if (fits_flush_file(m_fptr,&status)) throw FitsError(status);

               }


               HDUCreator create(this); 
               ExtHDU* readBack 
                = static_cast<ExtHDU*>(create.getHdu(tableName,readData,keyStore,
                                false,revisedVersion)); 
               return static_cast<Table&>(*addExtension(readBack)); 
       }
       catch (std::bad_cast&)
       {
               throw OperationNotSupported(" filtering rows from an image ");       
       }

       // output 


  }

  ExtHDU& FITS::currentExtension ()
  {
    // return the current extension. Must have been previously instantiated by
    // CCfits.
    int num(0);
    fits_get_hdu_num(m_fptr,&num);
    // extensions start counting at 1, HDU indices at 2.
    return extension(num - 1);
  }

  String FITS::nameOfUnmapped (int hduNum) const
  {
    // return the name of HDUs not yet mapped.    

    static char EXTNAME[] = "EXTNAME";
    static char HDUNAME[] = "HDUNAME";
    String retVal("");
    int current(0);
    int status(0);
    fits_get_hdu_num(m_fptr,&current);
    if (fits_movabs_hdu(m_fptr,hduNum,0,&status)) throw FitsError(status);

    FITSUtil::auto_array_ptr<char> pTestKey(new char[FLEN_VALUE]);
    char* testKey = pTestKey.get();

    if (fits_read_key_str(m_fptr,EXTNAME,testKey,0,&status) == 0)
    {
            retVal = String(testKey);
    }
    else 
    {   
        if (status == KEY_NO_EXIST)
        {
                status = 0;
                fits_read_key_str(m_fptr,HDUNAME,testKey,0,&status) ;
                if (status == 0) 
                {
                        retVal = String(testKey);
                }
                else 
                {
                        if (status != KEY_NO_EXIST) throw FitsError(status);
                }
        }
        else 
        {
                throw FitsError(status);
        }
    }
    if (fits_movabs_hdu(m_fptr,current,0,&status)) throw FitsError(status);

    return retVal;
  }

  void FITS::cloneHeader (const ExtHDU& source)
  {
    // raw copy header info from source to a new HDU in *this.

    // first,
    // check that source is current HDU in whichever file it is
    source.makeThisCurrent();

    fitsfile* sourcePointer = source.fitsPointer();
    int nKeys(0);
    int dum(0);
    int status(0);


    // get number of keys in source HDU.
    // don't care what key will be read next, so keynum argument is dummy.
    if (fits_get_hdrpos(sourcePointer,&nKeys,&dum,&status)) throw FitsError(status);
    FITSUtil::auto_array_ptr<char> pCard(new char[FLEN_CARD]);
    char* card = pCard.get();

    // Must copy the keys of the source before creating the new HDU
    // because one cannot movabs to a raw HDU created by create_hdu:
    // the required keywords are not there and it returns an error.
    if (sourcePointer == m_fptr)
    {
        std::vector<String> cards(nKeys);
        {     
                for (int j = 1; j <= nKeys; ++j)
                {
                        if (fits_read_record(sourcePointer,j,card,&status)) throw FitsError(status);  
                        cards[j-1] = String(card);     
                }
        }      
    // fits_create_hdu makes a new HDU and makes it current.
        if (fits_create_hdu(m_fptr,&status)) throw FitsError(status);
        {
                for (int j = 1; j <= nKeys; ++j)
                {
                        if (fits_write_record(m_fptr,const_cast<char*>(cards[j-1].c_str()),&status)) 
                                                                        throw FitsError(status);       
                }
        }  
    }

    else
    {
        if (fits_create_hdu(m_fptr,&status)) throw FitsError(status);
        for (int j = 1; j <= nKeys; ++j)
        {
                if (fits_read_record(sourcePointer,j,card,&status)) 
                        throw FitsError(status);       
                if (fits_write_record(m_fptr,card,&status)) 
                        throw FitsError(status);       
        }
    }
    // set the number of rows NAXIS2 in the new HDU back to zero - 
    // it's been raw copied from source.

    static char ROWS[] = "NAXIS2";
    if (fits_update_key_lng(m_fptr,ROWS,0,0,&status)) throw FitsError(status);
    if (fits_flush_file(m_fptr,&status)) throw FitsError(status);
  }

  void FITS::deleteExtension (int doomed)
  {
    int status(0);
    ExtHDU& d = extension(doomed); // throws NoSuchHDU if it's not there.
    const int removeIdx = d.index();
    std::vector<ExtHDU*> trailingExts;
    ExtMapConstIt itExtMap = m_extension.begin();
    ExtMapConstIt itExtMapEnd = m_extension.end();
    while (itExtMap != itExtMapEnd)
    {
       if (itExtMap->second->index() > removeIdx)
          trailingExts.push_back(itExtMap->second);
       ++itExtMap;
    }

    if (fits_delete_hdu(m_fptr,0,&status)) throw FitsError(status);
    unmapExtension(d);
    // Reindex the extensions that follow the deleted.
    for (size_t i=0; i<trailingExts.size(); ++i)
       trailingExts[i]->index(trailingExts[i]->index()-1);
  }

  void FITS::setCompressionType (int compType)
  {
     int status = 0;
     if (fits_set_compression_type(m_fptr, compType, &status))
     {
        throw FitsError(status);
     }
  }

  void FITS::setTileDimensions (const std::vector<long>& tileSizes)
  {
     int status = 0;
     int nDim = static_cast<int>(tileSizes.size());
     FITSUtil::CVarray<long> converter;
     FITSUtil::auto_array_ptr<long> pTileSizes(converter(tileSizes));
     if (fits_set_tile_dim(m_fptr, nDim, pTileSizes.get(), &status))
     {
        throw FitsError(status);
     }
     currentCompressionTileDim(nDim);
  }

  void FITS::setNoiseBits (int noiseBits)
  {
     int status = 0;
     if (fits_set_noise_bits(m_fptr, noiseBits, &status))
     {
        throw FitsError(status);
     }
  }

  int FITS::getCompressionType () const
  {
     int compType = 0;
     int status = 0;
     if (fits_get_compression_type(m_fptr, &compType, &status))
     {
        throw FitsError(status);
     }
     return compType;
  }

  void FITS::getTileDimensions (std::vector<long>& tileSizes) const
  {
     int status = 0;
     int nDim = currentCompressionTileDim();
     tileSizes.resize(nDim);

     if (nDim)
     {
        FITSUtil::auto_array_ptr<long> pTileSizes(new long[nDim]);
        long* tilesArray = pTileSizes.get();
        if (fits_get_tile_dim(m_fptr, nDim, tilesArray, &status))
        {
           throw FitsError(status);
        }
        std::copy(&tilesArray[0], &tilesArray[nDim], &tileSizes[0]);
     }
  }

  int FITS::getNoiseBits () const
  {
     int noiseBits = 0;
     int status = 0;
     if (fits_get_noise_bits(m_fptr, &noiseBits, &status))
     {
        throw FitsError(status);
     }
     return noiseBits;
  }

  ExtHDU* FITS::checkAlreadyRead(const int hduIdx, const String& hduName,
                        const int version) const throw()
  {
     // See header for description.
     ExtHDU* found=0;
     const ExtMap& extMap = m_extension;
     if (hduIdx)
     {
        ExtMapConstIt itMap = extMap.begin();
        const ExtMapConstIt itMapEnd = extMap.end();
        while (!found && itMap != itMapEnd)
        {
           if (itMap->second->index() == hduIdx)
              found = itMap->second;
           ++itMap;
        }
     }
     else
     {
        const std::pair<ExtMapConstIt,ExtMapConstIt> rangePair = 
                        extMap.equal_range(hduName);
        // Note itRange is not a reference but a copy.  We may want
        // to keep rangePair.first for later use.
        ExtMapConstIt itRange = rangePair.first;
        const ExtMapConstIt& itRangeEnd = rangePair.second;
        while (!found && itRange != itRangeEnd)
        {
           if (itRange->second->version() == version)
              found = itRange->second;
           ++itRange;
        }
        // This really shouldn't be needed in a well-structured FITS file, but
        // IF extension w/hduName exists, caller is requesting version 1, yet
        // no version 1 is found, THEN treat it as "return first found extension 
        // w/name".  This is mostly for backwards compatibility since I noticed 
        // earlier read(hduName) function code was doing this.
        if (!found && version == 1 && rangePair.first != itRangeEnd)
        {
           found = rangePair.first->second;
        }
     }
     return found;
  }

  // Additional Declarations

} // namespace CCfits
