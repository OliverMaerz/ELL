////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Machine Learning Library (EMLL)
//  File:     main.cpp (sgdTrainer)
//  Authors:  Ofer Dekel
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// utilities
#include "Files.h"
#include "OutputStreamImpostor.h" 
#include "CommandLineParser.h" 
#include "RandomEngines.h"
#include "Exception.h"

// dataset
#include "SupervisedExample.h"

// common
#include "SGDIncrementalTrainerArguments.h"
#include "MultiEpochIncrementalTrainerArguments.h"
#include "TrainerArguments.h"
#include "DataLoadArguments.h" 
#include "ModelLoadArguments.h"
#include "ModelSaveArguments.h"
#include "EvaluatorArguments.h"
#include "LoadModel.h"
#include "DataLoaders.h"
#include "MakeTrainer.h"
#include "MakeEvaluator.h"

// trainers
#include "SGDIncrementalTrainer.h"
#include "MultiEpochIncrementalTrainer.h"
#include "EvaluatingIncrementalTrainer.h"

// evaluators
#include "Evaluator.h"
#include "BinaryErrorAggregator.h"
#include "LossAggregator.h"

// lossFunctions
#include "HingeLoss.h"
#include "LogLoss.h"

// model
#include "Model.h"
#include "InputNode.h"

// nodes
#include "LinearPredictorNode.h"

// stl
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <memory>

int main(int argc, char* argv[])
{
    try
    {
        // create a command line parser
        utilities::CommandLineParser commandLineParser(argc, argv);

        // add arguments to the command line parser
        common::ParsedTrainerArguments trainerArguments;
        common::ParsedDataLoadArguments dataLoadArguments;
        common::ParsedModelSaveArguments modelSaveArguments;
        common::ParsedSGDIncrementalTrainerArguments sgdIncrementalTrainerArguments;
        common::ParsedMultiEpochIncrementalTrainerArguments multiEpochTrainerArguments;
        common::ParsedEvaluatorArguments evaluatorArguments;

        commandLineParser.AddOptionSet(trainerArguments);
        commandLineParser.AddOptionSet(dataLoadArguments);
        commandLineParser.AddOptionSet(modelSaveArguments);
        commandLineParser.AddOptionSet(multiEpochTrainerArguments);
        commandLineParser.AddOptionSet(sgdIncrementalTrainerArguments);
        commandLineParser.AddOptionSet(evaluatorArguments);

        // parse command line
        commandLineParser.Parse();

        if(trainerArguments.verbose)
        {
            std::cout << "Stochastic Gradient Descent Trainer" << std::endl;
            std::cout << commandLineParser.GetCurrentValuesString() << std::endl;
        }

        // load dataset
        if(trainerArguments.verbose) std::cout << "Loading data ..." << std::endl;
        auto rowDataset = common::GetRowDataset(dataLoadArguments);
        size_t numColumns = dataLoadArguments.parsedDataDimension;

        // predictor type
        using PredictorType = predictors::LinearPredictor;

        // create sgd trainer
        auto sgdIncrementalTrainer = common::MakeSGDIncrementalTrainer(numColumns, trainerArguments.lossArguments, sgdIncrementalTrainerArguments);

        // in verbose mode, create evaluator
        std::shared_ptr<evaluators::IEvaluator<PredictorType>> evaluator = nullptr;
        if(trainerArguments.verbose)
        {
            evaluator = common::MakeEvaluator<PredictorType>(rowDataset.GetIterator(), evaluatorArguments, trainerArguments.lossArguments);
            sgdIncrementalTrainer = std::make_unique<trainers::EvaluatingIncrementalTrainer<PredictorType>>(trainers::MakeEvaluatingIncrementalTrainer(std::move(sgdIncrementalTrainer), evaluator));
        }

        auto trainer = trainers::MakeMultiEpochIncrementalTrainer(std::move(sgdIncrementalTrainer), multiEpochTrainerArguments);

        // train
        if(trainerArguments.verbose) std::cout << "Training ..." << std::endl;
        auto trainSetIterator = rowDataset.GetIterator();
        trainer->Update(trainSetIterator);
        auto predictor = trainer->GetPredictor();

        // print loss and errors
        if(trainerArguments.verbose)
        {
            std::cout << "Finished training.\n";

            // print evaluation
            std::cout << "Training error\n";
            evaluator->Print(std::cout);
            std::cout << std::endl;
        }

        // save predictor model
        if(modelSaveArguments.outputModelFilename != "")
        {
            // Create a model
            model::Model model;
            auto inputNode = model.AddNode<model::InputNode<double>>(predictor->GetDimension());            
            model.AddNode<nodes::LinearPredictorNode>(inputNode->output, *predictor);
            common::SaveModel(model, modelSaveArguments.outputModelFilename);
        }
    }
    catch (const utilities::CommandLineParserPrintHelpException& exception)
    {
        std::cout << exception.GetHelpText() << std::endl;
        return 0;
    }
    catch (const utilities::CommandLineParserErrorException& exception)
    {
        std::cerr << "Command line parse error:" << std::endl;
        for (const auto& error : exception.GetParseErrors())
        {
            std::cerr << error.GetMessage() << std::endl;
        }
        return 1;
    }
    catch (const utilities::Exception& exception)
    {
        std::cerr << "exception: " << exception.GetMessage() << std::endl;
        return 1;
    }

    return 0;
}
