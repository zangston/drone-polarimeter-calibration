//	Astrophysics Science Division,
//	NASA/ Goddard Space Flight Center
//	HEASARC
//	http://heasarc.gsfc.nasa.gov
//	e-mail: ccfits@heasarc.gsfc.nasa.gov
//
//	Original author: Kristin Rutkowski


#include "GroupTable.h"
#include "ExtHDU.h"
#include "FITS.h"

#include <vector>

namespace CCfits {
  
  
//  GroupTable::GroupTable (const GroupTable & right)
//    : BinTable(right), 
//      m_name(right.m_name),
//      m_id(right.m_id),
//      m_numMembers(right.m_numMembers)
//  {
//    // +++ deep copy the members
//    
//  }
  
  
  // make a new group table
  GroupTable::GroupTable (FITS* p, int groupID, const String & groupName) 
    : BinTable(p, groupID, groupName),
      m_name(groupName),
      m_id(groupID),
      m_numMembers(0)
  { 
  }
  

  GroupTable::~GroupTable()
  {
  }
  
  
  HDU * GroupTable::addMember (HDU & newMember)
  {
    
    int status = 0;
    
    // add the member object ptr to the container of members
    m_members.push_back(&newMember);
    
    // now physically add a new row to the group table for this new member HDU
    // the hdupos param is only read if the new member file ptr is null
    if ( fits_add_group_member(fitsPointer(), newMember.fitsPointer(), 0, &status) ) throw FitsError(status);
    
    ++m_numMembers;
    
    // +++ not sure if this is what I should be doing here.  or just return void
    return &newMember;
    
  }
  
  
  HDU * GroupTable::addMember(int memberPosition)
  {
    
    int status = 0;
    
    // if using memberPosition, the new member HDU must be in the same file as
    // the current grouping table
    // +++ add this to documentation
    
    ExtHDU & newMember = parent()->extension(memberPosition);
    
    // +++ so we need to add an HDU * to the vector. what if there is another * to this HDU elsewhere?  Does it matter?
    // add the member object ptr to the container of members
    m_members.push_back(&newMember);
    
    // now physically add a new row to the group table for this new member HDU
    // the hdupos param is only read if the new member file ptr is null
    if ( fits_add_group_member(fitsPointer(), NULL, memberPosition , &status) ) throw FitsError(status);
    
    ++m_numMembers;
    
    // +++ not sure if this is what I should be doing here.  or just return void
    return &newMember;
    
  }
  
  void GroupTable::listMembers() const
  {
    
    std::cout << "Listing " << m_members.size() << " group members: " << std::endl;
    
    std::vector<HDU *>::const_iterator iter;
    for (iter = m_members.begin(); iter != m_members.end(); ++iter) {
      std::cout << "   " << dynamic_cast<ExtHDU *>(*iter)->name() << std::endl;
    }
    
  }
  
  
  //HDU * GroupTable::removeMember(HDU & member, bool deleteHDU)
  HDU * GroupTable::removeMember(HDU & member)
  {
    
    // +++ deleteHDU must be false for now
    bool deleteHDU = false;
    
    int status = 0;
    int rmopt = (deleteHDU ? OPT_RM_MBR : OPT_RM_ENTRY);
    long toRemove = 0;
    HDU * returnHDU = (deleteHDU ? NULL : &member);
    
    // +++ "updates the member's GRPIDn/GRPLCn keywords"
    //     so I should reread the keywords from this HDU (if it's not deleted),
    //     to update the datamembers?
    // +++ but there's a bug, and fits_remove_member doesn't actually remove the keys
    
    // remove the HDU from the list of group members
    // go through the vector of members and look for this one
    std::vector<HDU *>::iterator iter;
    for (iter = m_members.begin(); iter != m_members.end(); ++iter) {
      ++toRemove;
      if ( (**iter) == member ) {
        m_members.erase(iter);
        break;
      }
    }
    
    --m_numMembers;
    
    if (fits_remove_member(fitsPointer(), toRemove, rmopt, &status) ) throw FitsError(status);
    
    // +++ or should I just not return anything?
    // if we deleted the HDU, return a null pointer
    return returnHDU;
    
  }
  
  
  //HDU * GroupTable::removeMember(LONGLONG memberNumber, bool deleteHDU)
  HDU * GroupTable::removeMember(LONGLONG memberNumber)
  {
  
    // +++ deleteHDU must be false for now
    bool deleteHDU = false;
    
    int status = 0;
    int rmopt = (deleteHDU ? OPT_RM_MBR : OPT_RM_ENTRY);
    long toRemove = memberNumber;
    HDU * returnHDU = (deleteHDU ? NULL : *(m_members.begin()+memberNumber-1) );
    
    // +++ "updates the member's GRPIDn/GRPLCn keywords"
    //     so I should reread the keywords from this HDU (if it's not deleted),
    //     to update the datamembers?
    // +++ but there's a bug, and fits_remove_member doesn't actually remove the keys
    
    if ( (memberNumber <= 0) || (memberNumber > m_numMembers) ) {
      throw;
      // +++ better exception handling
    } else {
      returnHDU = *(m_members.begin()+memberNumber-1);
      m_members.erase(m_members.begin()+memberNumber-1);
    }
    
    
    --m_numMembers;
    
    if (fits_remove_member(fitsPointer(), toRemove, rmopt, &status) ) throw FitsError(status);
    
    // +++ or should I just not return anything?
    // if we deleted the HDU, return a null pointer
    return returnHDU;
    
  }
  
  
//  void GroupTable::mergeGroups (GroupTable & other, bool removeOther)
//  {
//    
//    int status = 0;
//    int mgopt = (removeOther ? OPT_MRG_MOV : OPT_MRG_COPY);
//    
      // +++ or have a func const getMembers() ?
//    // add all these new members to the current member vector
//    std::vector<HDU *>::iterator iter;
//    for (iter  = other.m_members().begin(); iter != other.m_members().end(); ++iter) {
//      addMember(**iter);
//    }
//    
//    if ( fits_merge_groups(other.fitsPointer(), fitsPointer(), mgopt, &status) ) throw FitsError(status);
//    
//    // " In both cases, the GRPIDn and GRPLCn keywords of the member HDUs are updated accordingly."
//    // +++ again, how should we handle the fact that the keywords in all these HDUs are updated?
//    //     if the HDUs are objects, we should reread the keywords, right?
//    
//    // +++ if removeOther, then the other HDU is deleted.  How should that be handled here?  
//    //     I don't like having extra functionality in a function
//    
//  }
  
  
//  void GroupTable::compactGroup(bool deleteSubGroupTables)
//  {
//    
//    int status = 0;
//    int cmopt = (deleteSubGroupTables ? OPT_CMT_MBR_DEL : OPT_CMT_MBR);
//    
//    // first, remove any group tables from the member vector
//    // +++ am I basically repeating the processing that is inside fits_compact_group ?  Since I don't keep detailed info about my members
//    
//    
//    // +++ this may delete HDUs that could possible have an object somewhere.  Do we care?
//    if (fits_compact_group(fitsPointer(), cmopt, &status) ) throw FitsError(status);
//    
//  }

  
//  bool GroupTable::verifyGroup () const 
//  {
//    
//    int status = 0;
//    long firstFailed = 0;
//    bool isVerified = false;
//    
//    if ( fits_verify_group(fitsPointer(), &firstFailed, &status) ) throw FitsError(status);
//    
//    // +++ I'm not sure how I want to handle this function
//    
//    if (firstFailed == 0) {
//      isVerified = true;
//    } else if (firstFailed > 0) {
//      // a member HDU failed
//      
//    } else {
//      // a group link failed
//      
//      
//    }
//    
//    return isVerified;
//    
//  }
  
} // namespace CCfits

