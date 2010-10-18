/* 
file name: gdcmPatientRootQuery.cpp
contains: a baseclass which will produce a dataset for c-find and c-move with patient root
name and date: 18 oct 2010 mmr

This class contains the functionality used in patient c-find and c-move queries.
StudyRootQuery derives from this class.

Namely:
1) list all tags associated with a particular query type
2) produce a query dataset via tag association

Eventually, it can be used to validate a particular dataset type.

The dataset held by this object (or, really, one of its derivates) should be passed to a c-find or c-move query.

*/

#include "gdcmPatientRootQuery.h"
#include "gdcmDataElement.h"
#include "gdcmTag.h"
#include "gdcmDict.h"
#include "gdcmDictEntry.h"
#include "gdcmDicts.h"
#include "gdcmGlobal.h"

namespace gdcm{
namespace network {

PatientRootQuery::PatientRootQuery(){
  mRootType = ePatientRootType;
  mHelpDescription = "Patient-level root query";
}
PatientRootQuery::~PatientRootQuery(){
  //nothing to do, really
}
ERootType PatientRootQuery::GetRootType() const{
  return mRootType;
}


void PatientRootQuery::SetSearchParameter(const gdcm::Tag& inTag, const gdcm::DictEntry& inDictEntry, const std::string& inValue){

  //borrowed this code from anonymization; not sure if it's correct, though.
  gdcm::DataElement de;
  de.SetTag( inTag );
  using gdcm::VR;
  const VR &vr = inDictEntry.GetVR();
  if( vr.IsDual() )
    {
    if( vr == VR::US_SS )
      {
      de.SetVR( VR::US );
      }
    else if( vr == VR::US_SS_OW )
      {
      de.SetVR( VR::OW );
      }
    else if( vr == VR::OB_OW )
      {
      de.SetVR( VR::OB );
      }
    }
  else
    {
    de.SetVR( vr );
    }

  mDataSet.Insert(de);
}

void PatientRootQuery::SetSearchParameter(const gdcm::Tag& inTag, const std::string& inValue){
  //IF WE WANTED, we could validate the incoming tag as belonging to our set of tags.
  //but we will not.

  static const gdcm::Global &g = gdcm::Global::GetInstance();
  static const gdcm::Dicts &dicts = g.GetDicts();
  static const gdcm::Dict &pubdict = dicts.GetPublicDict();

  const gdcm::DictEntry &dictentry = pubdict.GetDictEntry(inTag);

  SetSearchParameter(inTag, dictentry, inValue);

}
void PatientRootQuery::SetSearchParameter(const std::string& inKeyword, const std::string& inValue){

  static const gdcm::Global &g = gdcm::Global::GetInstance();
  static const gdcm::Dicts &dicts = g.GetDicts();
  static const gdcm::Dict &pubdict = dicts.GetPublicDict();

  gdcm::Tag theTag;
  const gdcm::DictEntry &dictentry = pubdict.GetDictEntryByName(inKeyword.c_str(), theTag);
  SetSearchParameter(theTag, dictentry, inValue);
}

const std::ostream &PatientRootQuery::WriteHelpFile(std::ostream &os){

  //mash all the query types into a vector for ease-of-use
  std::vector<QueryBase*> theQueries;
  std::vector<QueryBase*>::const_iterator qtor;
  theQueries.push_back(&mPatient);
  theQueries.push_back(&mStudy);
  theQueries.push_back(&mSeries);
  theQueries.push_back(&mImage);


  std::vector<gdcm::Tag> theTags;
  std::vector<gdcm::Tag>::iterator ttor;

  
  static const gdcm::Global &g = gdcm::Global::GetInstance();
  static const gdcm::Dicts &dicts = g.GetDicts();
  static const gdcm::Dict &pubdict = dicts.GetPublicDict();

  os << "The following tags must be supported by a C-FIND/C-MOVE " << mHelpDescription << ": " << std::endl;
  for (qtor = theQueries.begin(); qtor < theQueries.end(); qtor++){
    os << "Level: " << (*qtor)->GetName() << std::endl;
    theTags = (*qtor)->GetRequiredTags(GetRootType());
    for (ttor = theTags.begin(); ttor < theTags.end(); ttor++){
      const gdcm::DictEntry &dictentry = pubdict.GetDictEntry(*ttor);
      os << "Keyword: " << dictentry.GetKeyword() << " Tag: " << *ttor << std::endl;
    }
    os << std::endl;
  }

  
  os << std::endl;
  os << "The following tags are unique at each level of a " << mHelpDescription << ": " << std::endl;
  for (qtor = theQueries.begin(); qtor < theQueries.end(); qtor++){
    os << "Level: " << (*qtor)->GetName() << std::endl;
    theTags = (*qtor)->GetRequiredTags(GetRootType());
    for (ttor = theTags.begin(); ttor < theTags.end(); ttor++){
      const gdcm::DictEntry &dictentry = pubdict.GetDictEntry(*ttor);
      os << "Keyword: " << dictentry.GetKeyword() << " Tag: " << *ttor << std::endl;
    }
    os << std::endl;
  }

  
  os << std::endl;
  os << "The following tags are optional at each level of a " << mHelpDescription << ": "  << std::endl;
  for (qtor = theQueries.begin(); qtor < theQueries.end(); qtor++){
    os << "Level: " << (*qtor)->GetName() << std::endl;
    theTags = (*qtor)->GetRequiredTags(GetRootType());
    for (ttor = theTags.begin(); ttor < theTags.end(); ttor++){
      const gdcm::DictEntry &dictentry = pubdict.GetDictEntry(*ttor);
      os << "Keyword: " << dictentry.GetKeyword() << " Tag: " << *ttor << std::endl;
    }
    os << std::endl;
  }

  os << std::endl;

  return os;
}

DataSet PatientRootQuery::GetQueryDataSet() const{
  return mDataSet;
}

}
}