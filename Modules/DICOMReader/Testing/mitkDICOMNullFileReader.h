/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#ifndef mitkDICOMNullFileReader_h
#define mitkDICOMNullFileReader_h

#include "mitkDICOMFileReader.h"

namespace mitk
{

class DICOMNullFileReader : public DICOMFileReader
{
  public:

    mitkClassMacro( DICOMNullFileReader, DICOMFileReader );
    mitkCloneMacro( DICOMNullFileReader );
    itkNewMacro( DICOMNullFileReader );

    virtual void AnalyzeInputFiles();

    // void AllocateOutputImages();
    virtual bool LoadImages();

    virtual bool CanHandleFile(const std::string& filename);

  protected:

    DICOMNullFileReader();
    virtual ~DICOMNullFileReader();

    DICOMNullFileReader(const DICOMNullFileReader& other);
    DICOMNullFileReader& operator=(const DICOMNullFileReader& other);

  private:
};

}

#endif
