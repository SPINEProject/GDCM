/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library
  Module:  $URL$

  Copyright (c) 2006-2010 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRTStructSetProperties.h"
#include "vtkObjectFactory.h"
#include "vtkGDCMPolyDataWriter.h"
#include "vtkPolyData.h"

#include "gdcmDirectory.h"
#include "gdcmScanner.h"
#include "gdcmDataSet.h"
#include "gdcmReader.h"
#include "gdcmIPPSorter.h"
#include "gdcmAttribute.h"
#include "gdcmDirectoryHelper.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"

#include <string>
#include <map>
#include <vector>
#include <set>
#include <time.h> // for strftime
#include <ctype.h> // for isdigit
#include <assert.h>

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkRTStructSetProperties, "1.21")
vtkStandardNewMacro(vtkRTStructSetProperties)



//the function LoadCTImageFromFiles gives us a list of images in IPP-sorted order for
//constructing rt structures
//(that need both the z position and the appropriate SOP Instance UID for a plane)
//so, armed with that list of points and the list of images, the first point in
//each polydata object can be appropriately compiled into the rtstruct
//we use appendpolydata here-- each polydata is an organ
//NOTE: the number of outputs for the appendpolydata MUST MATCH the organ vectors!
vtkRTStructSetProperties* vtkRTStructSetProperties::ProduceStructureSetProperties(const std::string& inDirectory,
                                                                           const std::string& inStructLabel,
                                                                           const std::string& inStructName,
                                                                           vtkGDCMPolyDataWriter* inPolyData,//for polydata inputs
                                                                           gdcm::Directory::FilenamesType inROINames,
                                                                           gdcm::Directory::FilenamesType inROIAlgorithmName,
                                                                           gdcm::Directory::FilenamesType inROIType)
{
  using namespace gdcm;
  gdcm::Directory::FilenamesType theCTSeries =
    gdcm::DirectoryHelper::GetCTImageSeriesUIDs(inDirectory);
  if (theCTSeries.size() > 1){
    gdcmWarningMacro("More than one CT series detected, only reading series UID: " << theCTSeries[0]);
  }
  if (theCTSeries.empty()){
    gdcmWarningMacro("No CT Series found, trying MR.");
    theCTSeries = gdcm::DirectoryHelper::GetMRImageSeriesUIDs(inDirectory);
    if (theCTSeries.size() > 1){
      gdcmWarningMacro("More than one MR series detected, only reading series UID: " << theCTSeries[0]);
    }
    if (theCTSeries.empty()){
      gdcmWarningMacro("No CT or MR series found, throwing.");
      return NULL;
    }
  }
  //load the images in the CT series
  std::vector<DataSet> theCTDataSets = gdcm::DirectoryHelper::LoadImageFromFiles(inDirectory, theCTSeries[0]);
  if (theCTDataSets.empty()){
    gdcmWarningMacro("No CT or MR Images loaded, throwing.");
    return NULL;
  }

  //now, armed with this set of images, we can begin to properly construct the RTStructureSet
  vtkRTStructSetProperties* theRTStruct = vtkRTStructSetProperties::New();
  theRTStruct->SetStructureSetLabel(inStructLabel.c_str());
  theRTStruct->SetStructureSetName(inStructName.c_str());
  //theRTStruct->SetSOPInstanceUID(<#const char *_arg#>);//should be autogenerated by the object itself
  {
    const ByteValue* theValue = theCTDataSets[0].FindNextDataElement(Tag(0x0020,0x000d)).GetByteValue();
    std::string theStringValue(theValue->GetPointer(), theValue->GetLength());
    theRTStruct->SetStudyInstanceUID(theStringValue.c_str());
  }
  {
    const ByteValue* theValue = theCTDataSets[0].FindNextDataElement(Tag(0x0020,0x000e)).GetByteValue();
    std::string theStringValue(theValue->GetPointer(), theValue->GetLength());
    theRTStruct->SetReferenceSeriesInstanceUID(theStringValue.c_str());
  }
  {
    const ByteValue* theValue = theCTDataSets[0].FindNextDataElement(Tag(0x0020,0x0052)).GetByteValue();
    std::string theStringValue(theValue->GetPointer(), theValue->GetLength());
    theRTStruct->SetReferenceFrameOfReferenceUID(theStringValue.c_str());
  }
  //the series UID should be set automatically, and happen during creation
  //set the date and time to be now

  char date[22];
  const size_t datelen = 8;
  int res = System::GetCurrentDateTime(date);
  assert( res );
  (void)res;//warning removal
  //the date is the first 8 chars
  std::string dateString;
  dateString.insert(dateString.begin(), &(date[0]), &(date[datelen]));
  theRTStruct->SetStructureSetDate(dateString.c_str());
  std::string timeString;
  const size_t timelen = 6; //for now, only need hhmmss
  timeString.insert(timeString.begin(), &(date[datelen]), &(date[datelen+timelen]));
  theRTStruct->SetStructureSetTime(timeString.c_str());

  //for each image, we need to fill in the sop class and instance UIDs for the frame of reference
  std::string theSOPClassID = DirectoryHelper::GetSOPClassUID(theCTDataSets).c_str();
  for (unsigned long i = 0; i < theCTDataSets.size(); i++){
    theRTStruct->AddReferencedFrameOfReference(theSOPClassID.c_str(),
      DirectoryHelper::RetrieveSOPInstanceUIDFromIndex(i,theCTDataSets).c_str());
  }

  //now, we have go to through each vtkPolyData, assign the ROI names per polydata, and then also assign the
  //reference SOP instance UIDs on a per-plane basis.
  for (int j = 0; j < inPolyData->GetNumberOfInputPorts(); j++){
    theRTStruct->AddStructureSetROI(j,
      theRTStruct->GetReferenceFrameOfReferenceUID(),
      inROINames[j].c_str(),
      inROIAlgorithmName[j].c_str());
    theRTStruct->AddStructureSetROIObservation(j,
     j, inROIType[j].c_str(), "");
     //for each organ, gotta go through and add in the right planes in the
     //order that the tuples appear, as well as the colors
     //right now, each cell in the vtkpolydata is a contour in an xy plane
     //that's what MUST be passed in
    vtkPolyData* theData = inPolyData->GetInput(j);
    unsigned int cellnum = 0;
    vtkPoints *pts;
    vtkCellArray *polys;
    vtkIdType npts = 0;
    vtkIdType *indx = 0;
    pts = theData->GetPoints();
    polys = theData->GetPolys();
    double v[3];
    for (polys->InitTraversal(); polys->GetNextCell(npts,indx); cellnum++ ){
      if (npts < 1) continue;
      pts->GetPoint(indx[0],v);
      double theZ = v[2];
      std::string theSOPInstance = DirectoryHelper::RetrieveSOPInstanceUIDFromZPosition(theZ, theCTDataSets);
      //j is correct here, because it's adding, as in there's an internal vector
      //that's growing.
      theRTStruct->AddContourReferencedFrameOfReference(j,theSOPClassID.c_str(), theSOPInstance.c_str());
    }
  }


  return theRTStruct;
}
struct StructureSetROI
{
  int ROINumber;
  std::string RefFrameRefUID;
  std::string ROIName;
  std::string ROIGenerationAlgorithm;

  // (3006,0080) SQ (Sequence with undefine)# u/l, 1 RTROIObservationsSequence
  // (3006,0082) IS [0]                     #   2, 1 ObservationNumber
  // (3006,0084) IS [0]                     #   2, 1 ReferencedROINumber
  // (3006,00a4) CS [ORGAN]                 #   6, 1 RTROIInterpretedType
  // (3006,00a6) PN (no value available)    #   0, 0 ROIInterpreter
  int ObservationNumber;
  // RefROI is AFAIK simply ROINumber
  std::string RTROIInterpretedType;
  std::string ROIInterpreter;
};

//----------------------------------------------------------------------------
class vtkRTStructSetPropertiesInternals
{
public:
  void Print(ostream &os, vtkIndent indent)
    {
    }
  void DeepCopy(vtkRTStructSetPropertiesInternals *p)
    {
    ReferencedFrameOfReferences = p->ReferencedFrameOfReferences;
    }
  vtkIdType GetNumberOfContourReferencedFrameOfReferences()
    {
    return ContourReferencedFrameOfReferences.size();
    }
  vtkIdType GetNumberOfContourReferencedFrameOfReferences(vtkIdType pdnum)
    {
    return ContourReferencedFrameOfReferences[pdnum].size();
    }
  const char *GetContourReferencedFrameOfReferenceClassUID( vtkIdType pdnum, vtkIdType id )
    {
    return ContourReferencedFrameOfReferences[pdnum][ id ].first.c_str();
    }
  const char *GetContourReferencedFrameOfReferenceInstanceUID( vtkIdType pdnum, vtkIdType id )
    {
    return ContourReferencedFrameOfReferences[pdnum][ id ].second.c_str();
    }
  vtkIdType GetNumberOfReferencedFrameOfReferences()
    {
    return ReferencedFrameOfReferences.size();
    }
  const char *GetReferencedFrameOfReferenceClassUID( vtkIdType id )
    {
    return ReferencedFrameOfReferences[ id ].first.c_str();
    }
  const char *GetReferencedFrameOfReferenceInstanceUID(vtkIdType id )
    {
    return ReferencedFrameOfReferences[ id ].second.c_str();
    }
  void AddContourReferencedFrameOfReference( vtkIdType pdnum, const char *classuid , const char * instanceuid )
    {
    ContourReferencedFrameOfReferences.resize(pdnum+1);
    ContourReferencedFrameOfReferences[pdnum].push_back(
      std::make_pair( classuid, instanceuid ) );
    }
  std::vector< std::vector < std::pair< std::string, std::string > > > ContourReferencedFrameOfReferences;
  void AddReferencedFrameOfReference( const char *classuid , const char * instanceuid )
    {
    ReferencedFrameOfReferences.push_back(
      std::make_pair( classuid, instanceuid ) );
    }
  std::vector < std::pair< std::string, std::string > > ReferencedFrameOfReferences;
  void AddStructureSetROIObservation( int refnumber,
    int observationnumber,
    const char *rtroiinterpretedtype,
    const char *roiinterpreter
  )
    {
    std::vector<StructureSetROI>::iterator it = StructureSetROIs.begin();
    bool found = false;
    for( ; it != StructureSetROIs.end(); ++it )
      {
      if( it->ROINumber == refnumber )
        {
        found = true;
        it->ObservationNumber = observationnumber;
        it->RTROIInterpretedType = rtroiinterpretedtype;
        it->ROIInterpreter = roiinterpreter;
        }
      }
    // postcond
    assert( found );
    }

  void AddStructureSetROI( int roinumber,
    const char* refframerefuid,
    const char* roiname,
    const char* roigenerationalgorithm
  )
    {
    StructureSetROI structuresetroi;
    structuresetroi.ROIName = roiname;
    structuresetroi.ROINumber = roinumber;
    structuresetroi.RefFrameRefUID = refframerefuid;
    structuresetroi.ROIGenerationAlgorithm = roigenerationalgorithm;
    StructureSetROIs.push_back( structuresetroi );
    }
  vtkIdType GetNumberOfStructureSetROIs()
    {
    return StructureSetROIs.size();
    }
  int GetStructureSetObservationNumber(vtkIdType id)
    {
    return StructureSetROIs[id].ObservationNumber;
    }
  int GetStructureSetROINumber(vtkIdType id)
    {
    return StructureSetROIs[id].ROINumber;
    }
  const char *GetStructureSetRTROIInterpretedType(vtkIdType id)
    {
    return StructureSetROIs[id].RTROIInterpretedType.c_str();
    }
  const char *GetStructureSetROIRefFrameRefUID(vtkIdType id)
    {
    return StructureSetROIs[id].RefFrameRefUID.c_str();
    }
  const char *GetStructureSetROIName(vtkIdType id)
    {
    return StructureSetROIs[id].ROIName.c_str();
    }
  const char *GetStructureSetROIGenerationAlgorithm(vtkIdType id)
    {
    return StructureSetROIs[id].ROIGenerationAlgorithm.c_str();
    }

  std::vector<StructureSetROI> StructureSetROIs;
};

//----------------------------------------------------------------------------
vtkRTStructSetProperties::vtkRTStructSetProperties()
{
  this->Internals = new vtkRTStructSetPropertiesInternals;

  this->StructureSetLabel             = NULL;
  this->StructureSetName              = NULL;
  this->StructureSetDate              = NULL;
  this->StructureSetTime              = NULL;

  this->SOPInstanceUID= NULL;
  this->StudyInstanceUID = NULL;
  this->SeriesInstanceUID = NULL;
  this->ReferenceSeriesInstanceUID = NULL;
  this->ReferenceFrameOfReferenceUID = NULL;
}

//----------------------------------------------------------------------------
vtkRTStructSetProperties::~vtkRTStructSetProperties()
{
  delete this->Internals;
  this->Clear();
}

//----------------------------------------------------------------------------
void vtkRTStructSetProperties::AddContourReferencedFrameOfReference(vtkIdType pdnum, const char *classuid , const char * instanceuid )
{
  this->Internals->AddContourReferencedFrameOfReference(pdnum, classuid, instanceuid );
}
const char *vtkRTStructSetProperties::GetContourReferencedFrameOfReferenceClassUID( vtkIdType pdnum, vtkIdType id )
{
  return this->Internals->GetContourReferencedFrameOfReferenceClassUID(pdnum, id );
}

const char *vtkRTStructSetProperties::GetContourReferencedFrameOfReferenceInstanceUID( vtkIdType pdnum, vtkIdType id )
{
  return this->Internals->GetContourReferencedFrameOfReferenceInstanceUID(pdnum ,id );
}

vtkIdType vtkRTStructSetProperties::GetNumberOfContourReferencedFrameOfReferences()
{
  return this->Internals->GetNumberOfContourReferencedFrameOfReferences();
}

vtkIdType vtkRTStructSetProperties::GetNumberOfContourReferencedFrameOfReferences(vtkIdType pdnum)
{
  return this->Internals->GetNumberOfContourReferencedFrameOfReferences(pdnum);
}

void vtkRTStructSetProperties::AddReferencedFrameOfReference( const char *classuid , const char * instanceuid )
{
  this->Internals->AddReferencedFrameOfReference( classuid, instanceuid );
}

const char *vtkRTStructSetProperties::GetReferencedFrameOfReferenceClassUID( vtkIdType id )
{
  return this->Internals->GetReferencedFrameOfReferenceClassUID(id );
}

const char *vtkRTStructSetProperties::GetReferencedFrameOfReferenceInstanceUID( vtkIdType id )
{
  return this->Internals->GetReferencedFrameOfReferenceInstanceUID(id );
}

vtkIdType vtkRTStructSetProperties::GetNumberOfReferencedFrameOfReferences()
{
  return this->Internals->GetNumberOfReferencedFrameOfReferences();
}
void vtkRTStructSetProperties::AddStructureSetROIObservation( int refnumber,
    int observationnumber,
    const char *rtroiinterpretedtype,
    const char *roiinterpreter)
{
  this->Internals->AddStructureSetROIObservation( refnumber, observationnumber, rtroiinterpretedtype, roiinterpreter );
}

void vtkRTStructSetProperties::AddStructureSetROI( int roinumber,
    const char* refframerefuid,
    const char* roiname,
    const char* roigenerationalgorithm
  )
{
  this->Internals->AddStructureSetROI( roinumber, refframerefuid, roiname, roigenerationalgorithm );
}

vtkIdType vtkRTStructSetProperties::GetNumberOfStructureSetROIs()
{
  return this->Internals->GetNumberOfStructureSetROIs();
}
int vtkRTStructSetProperties::GetStructureSetObservationNumber(vtkIdType id)
{
  return this->Internals->GetStructureSetObservationNumber(id);
}
int vtkRTStructSetProperties::GetStructureSetROINumber(vtkIdType id)
{
  return this->Internals->GetStructureSetROINumber(id);
}
const char *vtkRTStructSetProperties::GetStructureSetRTROIInterpretedType(vtkIdType id)
{
  return this->Internals->GetStructureSetRTROIInterpretedType(id);
}

const char *vtkRTStructSetProperties::GetStructureSetROIRefFrameRefUID(vtkIdType id)
{
  return this->Internals->GetStructureSetROIRefFrameRefUID(id);
}
const char *vtkRTStructSetProperties::GetStructureSetROIName(vtkIdType id)
{
  return this->Internals->GetStructureSetROIName(id);
}
const char *vtkRTStructSetProperties::GetStructureSetROIGenerationAlgorithm(vtkIdType id)
{
  return this->Internals->GetStructureSetROIGenerationAlgorithm(id);
}

//----------------------------------------------------------------------------
void vtkRTStructSetProperties::Clear()
{
  this->SetStructureSetLabel(NULL);
  this->SetStructureSetName(NULL);
  this->SetStructureSetDate(NULL);
  this->SetStructureSetTime(NULL);

  this->SetSOPInstanceUID( NULL );
  this->SetStudyInstanceUID ( NULL );
  this->SetSeriesInstanceUID ( NULL );
  this->SetReferenceSeriesInstanceUID ( NULL );
  this->SetReferenceFrameOfReferenceUID ( NULL );

}

//----------------------------------------------------------------------------
void vtkRTStructSetProperties::DeepCopy(vtkRTStructSetProperties *p)
{
  if (p == NULL)
    {
    return;
    }

  this->Clear();

  this->SetStructureSetDate(p->GetStructureSetDate());
  this->SetStructureSetTime(p->GetStructureSetTime());

  this->Internals->DeepCopy( p->Internals );
}

//----------------------------------------------------------------------------
void vtkRTStructSetProperties::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "\n" << indent << "StructureSetDate: ";
  if (this->StructureSetDate)
    {
    os << this->StructureSetDate;
    }
  os << "\n" << indent << "StructureSetTime: ";
  if (this->StructureSetTime)
    {
    os << this->StructureSetTime;
    }

  this->Internals->Print(os << "\n", indent.GetNextIndent() );
}