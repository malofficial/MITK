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

#include <mitkBaseData.h>
#include <mitkImageCast.h>
#include <mitkImageToItk.h>
#include <metaCommand.h>
#include <mitkCommandLineParser.h>
#include <usAny.h>
#include <mitkIOUtil.h>
#include <itksys/SystemTools.hxx>
#include <itkDirectory.h>
#include <mitkFiberBundle.h>
#include <mitkPreferenceListReaderOptionsFunctor.h>
#include <itkImageFileWriter.h>
#include <mitkPeakImage.h>
#include <itkFitFibersToImageFilter.h>

using namespace std;
typedef itksys::SystemTools ist;
typedef itk::Point<float, 4> PointType4;
typedef itk::Image< float, 4 >  PeakImgType;

std::vector< string > get_file_list(const std::string& path)
{
  std::vector< string > file_list;
  itk::Directory::Pointer dir = itk::Directory::New();

  if (dir->Load(path.c_str()))
  {
    int n = dir->GetNumberOfFiles();
    for (int r = 0; r < n; r++)
    {
      const char *filename = dir->GetFile(r);
      std::string ext = ist::GetFilenameExtension(filename);
      if (ext==".fib" || ext==".trk")
        file_list.push_back(path + '/' + filename);
    }
  }
  return file_list;
}

/*!
\brief Fits the tractogram to the input peak image by assigning a weight to each fiber (similar to https://doi.org/10.1016/j.neuroimage.2015.06.092).
*/
int main(int argc, char* argv[])
{
  mitkCommandLineParser parser;

  parser.setTitle("Fit Fibers To Image");
  parser.setCategory("Fiber Tracking and Processing Methods");
  parser.setDescription("Assigns a weight to each fiber in order to optimally explain the input peak image");
  parser.setContributor("MIC");

  parser.setArgumentPrefix("--", "-");
  parser.addArgument("", "i1", mitkCommandLineParser::StringList, "Input tractograms:", "input tractograms (.fib, vtk ascii file format)", us::Any(), false);
  parser.addArgument("", "i2", mitkCommandLineParser::InputFile, "Input peaks:", "input peak image", us::Any(), false);
  parser.addArgument("", "o", mitkCommandLineParser::OutputDirectory, "Output:", "output root", us::Any(), false);

  parser.addArgument("max_iter", "", mitkCommandLineParser::Int, "Max. iterations:", "maximum number of optimizer iterations", 20);
  parser.addArgument("bundle_based", "", mitkCommandLineParser::Bool, "Bundle based fit:", "fit one weight per input tractogram/bundle, not for each fiber", false);
  parser.addArgument("min_g", "", mitkCommandLineParser::Float, "Min. g:", "lower termination threshold for gradient magnitude", 1e-5);
  parser.addArgument("lambda", "", mitkCommandLineParser::Float, "Lambda:", "modifier for regularization", 0.1);
  parser.addArgument("save_res", "", mitkCommandLineParser::Bool, "Residuals:", "save residual images", false);
  parser.addArgument("dont_filter_outliers", "", mitkCommandLineParser::Bool, "Don't filter outliers:", "don't perform second optimization run with an upper weight bound based on the first weight estimation (95% quantile)", false);

  map<string, us::Any> parsedArgs = parser.parseArguments(argc, argv);
  if (parsedArgs.size()==0)
    return EXIT_FAILURE;

  mitkCommandLineParser::StringContainerType fib_files = us::any_cast<mitkCommandLineParser::StringContainerType>(parsedArgs["i1"]);
  string peak_file_name = us::any_cast<string>(parsedArgs["i2"]);
  string outRoot = us::any_cast<string>(parsedArgs["o"]);

  bool single_fib = true;
  if (parsedArgs.count("bundle_based"))
    single_fib = !us::any_cast<bool>(parsedArgs["bundle_based"]);

  bool save_residuals = false;
  if (parsedArgs.count("save_res"))
    save_residuals = us::any_cast<bool>(parsedArgs["save_res"]);

  int max_iter = 20;
  if (parsedArgs.count("max_iter"))
    max_iter = us::any_cast<int>(parsedArgs["max_iter"]);

  float g_tol = 1e-5;
  if (parsedArgs.count("min_g"))
    g_tol = us::any_cast<float>(parsedArgs["min_g"]);

  float lambda = 0.1;
  if (parsedArgs.count("lambda"))
    lambda = us::any_cast<float>(parsedArgs["lambda"]);

  bool filter_outliers = true;
  if (parsedArgs.count("dont_filter_outliers"))
    filter_outliers = !us::any_cast<bool>(parsedArgs["dont_filter_outliers"]);

  try
  {
    std::vector< mitk::FiberBundle::Pointer > input_tracts;

    mitk::PreferenceListReaderOptionsFunctor functor = mitk::PreferenceListReaderOptionsFunctor({"Peak Image", "Fiberbundles"}, {});
    mitk::Image::Pointer inputImage = dynamic_cast<mitk::PeakImage*>(mitk::IOUtil::Load(peak_file_name, &functor)[0].GetPointer());

    typedef mitk::ImageToItk< PeakImgType > CasterType;
    CasterType::Pointer caster = CasterType::New();
    caster->SetInput(inputImage);
    caster->Update();
    PeakImgType::Pointer peak_image = caster->GetOutput();

    std::vector< std::string > fib_names;
    for (auto item : fib_files)
    {
      if ( ist::FileIsDirectory(item) )
      {
        for ( auto fibFile : get_file_list(item) )
        {
          mitk::FiberBundle::Pointer inputTractogram = dynamic_cast<mitk::FiberBundle*>(mitk::IOUtil::Load(fibFile)[0].GetPointer());
          if (inputTractogram.IsNull())
            continue;
          input_tracts.push_back(inputTractogram);
          fib_names.push_back(fibFile);
        }
      }
      else
      {
        mitk::FiberBundle::Pointer inputTractogram = dynamic_cast<mitk::FiberBundle*>(mitk::IOUtil::Load(item)[0].GetPointer());
        if (inputTractogram.IsNull())
          continue;
        input_tracts.push_back(inputTractogram);
        fib_names.push_back(item);
      }
    }

    itk::FitFibersToImageFilter::Pointer fitter = itk::FitFibersToImageFilter::New();
    fitter->SetPeakImage(peak_image);
    fitter->SetTractograms(input_tracts);
    fitter->SetFitIndividualFibers(single_fib);
    fitter->SetMaxIterations(max_iter);
    fitter->SetGradientTolerance(g_tol);
    fitter->SetLambda(lambda);
    fitter->SetFilterOutliers(filter_outliers);
    fitter->Update();

    if (save_residuals)
    {
      itk::ImageFileWriter< PeakImgType >::Pointer writer = itk::ImageFileWriter< PeakImgType >::New();
      writer->SetInput(fitter->GetFittedImage());
      writer->SetFileName(outRoot + "fitted_image.nrrd");
      writer->Update();

      writer->SetInput(fitter->GetResidualImage());
      writer->SetFileName(outRoot + "residual_image.nrrd");
      writer->Update();

      writer->SetInput(fitter->GetOverexplainedImage());
      writer->SetFileName(outRoot + "overexplained_image.nrrd");
      writer->Update();

      writer->SetInput(fitter->GetUnderexplainedImage());
      writer->SetFileName(outRoot + "underexplained_image.nrrd");
      writer->Update();
    }

    std::vector< mitk::FiberBundle::Pointer > output_tracts = fitter->GetTractograms();
    for (unsigned int bundle=0; bundle<output_tracts.size(); bundle++)
    {
      std::string name = fib_names.at(bundle);
      name = ist::GetFilenameWithoutExtension(name);
      mitk::IOUtil::Save(output_tracts.at(bundle), outRoot + name + "_fitted.fib");
    }
  }
  catch (itk::ExceptionObject e)
  {
    std::cout << e;
    return EXIT_FAILURE;
  }
  catch (std::exception e)
  {
    std::cout << e.what();
    return EXIT_FAILURE;
  }
  catch (...)
  {
    std::cout << "ERROR!?!";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
