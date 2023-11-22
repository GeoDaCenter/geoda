#include <cassert>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <zlib.h>
#include "MatfileReader.h"

// endian_swap utility
inline void endianSwap(uint16_t& x) {
    x = (x>>8) | 
        (x<<8);
}
inline void endianSwap(uint32_t& x) {
    x = (x>>24) | 
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
}
inline void endianSwap(uint64_t& x) {
    x = (x>>56) | 
        ((x<<40) & 0x00FF000000000000) |
        ((x<<24) & 0x0000FF0000000000) |
        ((x<<8)  & 0x000000FF00000000) |
        ((x>>8)  & 0x00000000FF000000) |
        ((x>>24) & 0x0000000000FF0000) |
        ((x>>40) & 0x000000000000FF00) |
        (x<<56);
}

//=========================================
std::pair<EDataType, EMatrixClass> classify(char* data, bool endianSwap) {
    uint32_t dataType;
    bool isSmallDataElement = (data[2] & 0xFF) || (data[3] & 0xFF);
    if (isSmallDataElement)
        dataType = *(reinterpret_cast<uint16_t*>(data));
    else
        dataType = *(reinterpret_cast<uint32_t*>(data));
    if (static_cast<EDataType>(dataType) != miMATRIX)
        return std::make_pair(static_cast<EDataType>(dataType), mxINVALID);
    // miMATRIX must not be small element
    //assert(!isSmallDataElement);

    ArrayFlags* af = new ArrayFlags(data+8, endianSwap);

    EMatrixClass matrixClass;
    matrixClass = af->klass();
    return std::make_pair(static_cast<EDataType>(dataType), matrixClass);
}

DataElement* parse(char* data, bool endianSwap) {
    DataElement* de = NULL;
    // dispatch
    std::pair<EDataType, EMatrixClass> kind = classify(data, endianSwap);
    EDataType dataType = kind.first;
    EMatrixClass matrixClass = kind.second;
    switch (dataType) {
    case miINT8:
        de = new FlatDataElement<int8_t>(data, endianSwap);
        break;
    case miUINT8:
        de = new FlatDataElement<uint8_t>(data, endianSwap);
        break;
    case miINT16:
        de = new FlatDataElement<int16_t>(data, endianSwap);
        break;
    case miUINT16:
        de = new FlatDataElement<uint16_t>(data, endianSwap);
        break;
    case miINT32:
        de = new FlatDataElement<int32_t>(data, endianSwap);
        break;
    case miUINT32:
        de = new FlatDataElement<uint32_t>(data, endianSwap);
        break;
    case miINT64:
        de = new FlatDataElement<int64_t>(data, endianSwap);
        break;
    case miUINT64:
        de = new FlatDataElement<uint64_t>(data, endianSwap);
        break;
    case miSINGLE:
        de = new FlatDataElement<float>(data, endianSwap);
        break;
    case miDOUBLE:
        de = new FlatDataElement<double>(data, endianSwap);
        break;
    case miCOMPRESSED:
        de = new CompressedDataElement(data, endianSwap);
        break;
    case miUTF8:
        de = new UnicodeDataElement<miUTF8>(data, endianSwap);
        break;
    case miUTF16:
        de = new UnicodeDataElement<miUTF16>(data, endianSwap);
        break;
    case miUTF32:
        de = new UnicodeDataElement<miUTF32>(data, endianSwap);
        break;
    case miMATRIX:
        switch (matrixClass) {
        case mxCELL_CLASS:
            de = new Cell(data, endianSwap);
            break;
        case mxSTRUCT_CLASS:
            de = new Struct(data, endianSwap);
            break;
        case mxOBJECT_CLASS:
            de = new Object(data, endianSwap);
            break;
        case mxCHAR_CLASS:
            de = new NumericArray<char>(data, endianSwap);
            break;
        case mxSPARSE_CLASS:
            de = new SparseArray<double>(data, endianSwap);
            break;
        case mxDOUBLE_CLASS:
            de = new NumericArray<double>(data, endianSwap);
            break;
        case mxSINGLE_CLASS:
            de = new NumericArray<float>(data, endianSwap);
            break;
        case mxINT8_CLASS:
            de = new NumericArray<int8_t>(data, endianSwap);
            break;
        case mxUINT8_CLASS:
            de = new NumericArray<uint8_t>(data, endianSwap);
            break;
        case mxINT16_CLASS:
            de = new NumericArray<int8_t>(data, endianSwap);
            break;
        case mxUINT16_CLASS:
            de = new NumericArray<uint8_t>(data, endianSwap);
            break;
        case mxINT32_CLASS:
            de = new NumericArray<int8_t>(data, endianSwap);
            break;
        case mxUINT32_CLASS:
            de = new NumericArray<uint8_t>(data, endianSwap);
            break;
        case mxINT64_CLASS:
            de = new NumericArray<int8_t>(data, endianSwap);
            break;
        case mxUINT64_CLASS:
            de = new NumericArray<uint8_t>(data, endianSwap);
            break;
        default:
            std::cerr << "invalid EMatrixClass!\n";
        }
        break;
    default:
        std::cerr << "invalid EDataType!\n";
    }
    if (!de) {
        std::cerr << "failed to parse!\n";
    }
    return de;
}
//===========================================================

void DataElement::parseTag(char* tag) {
    _isSmallDataElement = (tag[2] & 0xFF) || (tag[3] & 0xFF);
    if (_isSmallDataElement) {
        _dataType = static_cast<EDataType>(*(reinterpret_cast<uint16_t*>(tag)));
        _numberOfBytes = *(reinterpret_cast<uint16_t*>(tag+2));
    } else {
        _dataType = static_cast<EDataType>(*(reinterpret_cast<uint32_t*>(tag)));
        _numberOfBytes = *(reinterpret_cast<uint32_t*>(tag+4));
    }
}

template <typename T>
FlatDataElement<T>::FlatDataElement(char* data, bool endianSwap) : DataElement(endianSwap), _data() {
    parseTag(data);
    if (_isSmallDataElement)
        data += 4;
    else
        data += 8;
    T* p = reinterpret_cast<T*>(data);
    int size = _numberOfBytes / sizeof(T);
    _data.reserve(size);
    _data.insert(_data.begin(), p, p+size);
}

FieldNames::FieldNames(char* data, bool endianSwap, int32_t fieldNameLength) :
    FlatDataElement<int8_t>(data, endianSwap), _fieldNameLength(fieldNameLength), _fieldNames() {
    std::string name;
    for (std::vector<int8_t>::iterator iter = _data.begin();
         iter + _fieldNameLength <= _data.end();
         iter += _fieldNameLength) {
        name.assign(iter, iter+_fieldNameLength);
        _fieldNames.push_back(name);
    }
}

CompressedDataElement::CompressedDataElement(char* data, bool endianSwap) :
    _decompressedData(NULL), DataElement(endianSwap) {
    parseTag(data);
    if (_isSmallDataElement)
        data += 4;              // not likely to happen
    else
        data += 8;
    unsigned long size = _numberOfBytes * 2; // 4x as a hint
    unsigned long rsize = 0;
    int res = Z_BUF_ERROR;
    while (res != Z_OK) {
        if (res == Z_BUF_ERROR) {
            if (_decompressedData) {
                delete[] _decompressedData;
            }
            size *= 2;
            _decompressedData = new char[size];
            if (!_decompressedData) {
                std::cerr << "FlatDataElement::parseData new failed!\n";
            }
            rsize = size;
            res = uncompress(reinterpret_cast<unsigned char*>(_decompressedData), &size,
                             reinterpret_cast<unsigned char*>(data), _numberOfBytes);
        } else {
            std::cerr << "FlatDataElement::parseData uncompress failed!\n";
        }
    }
    _decompressedSize = static_cast<uint32_t>(rsize);
}

DataElement* CompressedDataElement::reparse() {
    DataElement* res = parse(_decompressedData, _endianSwap);
    return res;
}

template <typename T>
NumericArray<T>::NumericArray(char* data, bool endianSwap) :
    MatrixDataElement(endianSwap), _real(NULL), _imag(NULL)  {
    char* start = data;
    parseTag(data);
    if (_isSmallDataElement)
        data += 4;              // not likely to happen
    else
        data += 8;
    parseFlags(data);
    data += _flags->totalBytes();
    parseDimensionsArray(data);
    data += _dimensionsArray->totalBytes();
    parseArrayName(data);
    data += _name->totalBytes();
    parseReal(data);
    data += _real->totalBytes();
    if (data < start + _numberOfBytes + 8) {
        parseImag(data);
        data += _imag->totalBytes();
    }
    //assert(data == start+_numberOfBytes+8);
}

template <typename T>
SparseArray<T>::SparseArray(char* data, bool endianSwap) :
    MatrixDataElement(endianSwap), _rows(NULL), _cols(NULL), _real(NULL), _imag(NULL) {
    char* start = data;
    parseTag(data);
    if (_isSmallDataElement)
        data += 4;              // not likely to happen
    else
        data += 8;
    parseFlags(data);
    data += _flags->totalBytes();
    parseDimensionsArray(data);
    data += _dimensionsArray->totalBytes();
    parseArrayName(data);
    data += _name->totalBytes();
    parseRows(data);
    data += _rows->totalBytes();
    parseCols(data);
    data += _cols->totalBytes();
    parseReal(data);
    data += _real->totalBytes();
    if (data < start + _numberOfBytes + 8) {
        parseImag(data);
        data += _imag->totalBytes();
    }
    //assert(data == start+_numberOfBytes+8);
}

Cell::Cell(char* data, bool endianSwap) : MatrixDataElement(endianSwap), _cells() {
    char* start = data;
    parseTag(data);
    if (_isSmallDataElement)
        data += 4;              // not likely to happen
    else
        data += 8;
    parseFlags(data);
    data += _flags->totalBytes();
    parseDimensionsArray(data);
    data += _dimensionsArray->totalBytes();
    parseArrayName(data);
    data += _name->totalBytes();
    MatrixDataElement* cell = NULL;
    while(data < start + _numberOfBytes + 8) {
        // create matrix and add to _cells
        cell = dynamic_cast<MatrixDataElement*>(parse(data, endianSwap));
        //assert(cell);
        _cells.push_back(cell);
        data += cell->totalBytes();
    }
    //assert(data == start+_numberOfBytes+8);
}

Struct::Struct(char* data, bool endianSwap) :
    MatrixDataElement(endianSwap), _fieldNameLength(NULL), _fieldNames(NULL), _fields() {
    char* start = data;
    parseTag(data);
    if (_isSmallDataElement)
        data += 4;              // not likely to happen
    else
        data += 8;
    parseFlags(data);
    data += _flags->totalBytes();
    parseDimensionsArray(data);
    data += _dimensionsArray->totalBytes();
    parseArrayName(data);
    data += _name->totalBytes();
    parseFieldNameLength(data);
    data += _fieldNameLength->totalBytes();
    parseFieldNames(data);
    data += _fieldNames->totalBytes();
    MatrixDataElement* field = NULL;
    while(data < start + _numberOfBytes + 8) {
        // create matrix and add to _fields
        field = dynamic_cast<MatrixDataElement*>(parse(data, endianSwap));
        //assert(field);
        _fields.push_back(field);
        data += field->totalBytes();
    }
    //assert(data == start+_numberOfBytes+8);
}

Object::Object(char* data, bool endianSwap) : _className(NULL), Struct(endianSwap) {
    char* start = data;
    parseTag(data);
    if (_isSmallDataElement)
        data += 4;              // not likely to happen
    else
        data += 8;
    parseFlags(data);
    data += _flags->totalBytes();
    parseDimensionsArray(data);
    data += _dimensionsArray->totalBytes();
    parseArrayName(data);
    data += _name->totalBytes();
    parseClassName(data);
    data += _className->totalBytes();
    parseFieldNameLength(data);
    data += _fieldNameLength->totalBytes();
    parseFieldNames(data);
    data += _fieldNames->totalBytes();
    MatrixDataElement* field = NULL;
    while(data < start + _numberOfBytes + 8) {
        // create matrix and add to _fields
        field = dynamic_cast<MatrixDataElement*>(parse(data, endianSwap));
        //assert(field);
        _fields.push_back(field);
        data += field->totalBytes();
    }
    //assert(data == start+_numberOfBytes+8);
}


MatfileReader::MatfileReader(std::ifstream& inputStream)
    : _matfile(""), _inputStream(inputStream), _descriptiveText(), _subsysDataOffset(0),
      _version(0), _endianIndicator(), _endianSwap(false) {
    //_inputStream.open(_matfile.c_str(), ios_base::in | ios_base::binary);
    if(!_inputStream.is_open()) {
        std::cerr << "open " << _matfile.c_str() << " error!\n";
    }
}

MatfileReader::~MatfileReader() {
    for (int i = 0; i < _dataElements.size(); ++i)  {
        delete _dataElements[i];
    }
}

void MatfileReader::parseHeader() {
    gotoHeader();
    _inputStream.read(_header, 128);
    _descriptiveText.assign(_header, 116);
    memcpy(&_subsysDataOffset, _header+116, 8);
    memcpy(&_version, _header+124, 2);
    _endianIndicator.assign(_header+126, 2);
    if (_endianIndicator.compare("IM")) {
        _endianSwap = true;
    } else {
        _endianSwap = false;
    }
}

bool MatfileReader::parseDataElement() {
    if (_inputStream.eof())
        return false;
    uint32_t dataType;
    uint32_t numberOfBytes;
    bool isSmallDataElement;
    char tag[8];
    _inputStream.read(reinterpret_cast<char*>(&tag[0]), 4);
    isSmallDataElement = tag[0] && tag[1]; // test the lower two bytes
    if (isSmallDataElement) {
        dataType = *(reinterpret_cast<uint16_t*>(&tag[2]));
        numberOfBytes = *(reinterpret_cast<uint16_t*>(&tag[0]));
    } else {
        dataType = *(reinterpret_cast<uint32_t*>(&tag[0]));
        _inputStream.read(reinterpret_cast<char*>(&tag[4]), 4);
        numberOfBytes = *(reinterpret_cast<uint32_t*>(&tag[4]));
    }
    char* data = NULL;
    DataElement* de = NULL;
    if (isSmallDataElement) {
        de = parse(tag, _endianSwap);
        if (de == NULL) {
            return false;
        }
    } else {
        data = new char[numberOfBytes+8];
        if (!data) {
            //cerr << "MatfileReader::parseDataElement new failed!\n";
            return false;
        }
        memcpy(data, tag, 8);
        _inputStream.read(reinterpret_cast<char*>(data+8), numberOfBytes);
        de = parse(data, _endianSwap);
    }
    //assert(de);
    if (de == NULL) {
        return false;
    }
    _dataElements.push_back(de);
    // padding
    if (numberOfBytes % 8) {
        _inputStream.ignore(8 - numberOfBytes % 8);
    }
    return true;
}

void MatfileReader::parseAllDataElements() {
    gotoData();
    while (!_inputStream.eof()) {
        parseDataElement();
    }
}
