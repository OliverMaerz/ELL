// DatasetLoader.h

#pragma once

#include "SupervisedExample.h"

#include "RowMatrix.h"
using linear::RowMatrix;

#include<string>
using std::string;

namespace common
{
    /// Container that holds a static function that loads and parses a dataset 
    ///
    struct DatasetLoader // TODO make this a standard function, not a static member
    {
        template<typename RowIteratorType, typename VectorEntryParserType>
        static RowDataset Load(RowIteratorType line_iterator, VectorEntryParserType parser);
    };
}

#include "../tcc/DatasetLoader.tcc"
