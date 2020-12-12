#ifndef _MATFILEREADER_H_
#define _MATFILEREADER_H_

#if defined(__APPLE__) || defined(__linux__)
#include <stdint.h>
#else
#include <cstdint>
#endif

#include <iosfwd>
#include <fstream>
#include <vector>
#include <cstring>

using namespace std;

enum EDataType {
    miINT8 = 1,
    miUINT8 = 2,
    miINT16 = 3,
    miUINT16 = 4,
    miINT32 = 5,
    miUINT32 = 6,
    miSINGLE = 7,
    miDOUBLE = 9,
    miINT64 = 12,
    miUINT64 = 13,
    miMATRIX = 14,
    miCOMPRESSED = 15,
    miUTF8 = 16,
    miUTF16 = 17,
    miUTF32 = 18,
    miINVALID = 0,
};

enum EMatrixClass {
    mxCELL_CLASS = 1,
    mxSTRUCT_CLASS = 2,
    mxOBJECT_CLASS = 3,
    mxCHAR_CLASS = 4,
    mxSPARSE_CLASS = 5,
    mxDOUBLE_CLASS = 6,
    mxSINGLE_CLASS = 7,
    mxINT8_CLASS = 8,
    mxUINT8_CLASS = 9,
    mxINT16_CLASS = 10,
    mxUINT16_CLASS = 11,
    mxINT32_CLASS = 12,
    mxUINT32_CLASS = 13,
    mxINT64_CLASS = 14,
    mxUINT64_CLASS = 15,
    mxINVALID = 0,
};

std::pair<EDataType, EMatrixClass> classify(char* data);
class DataElement;
DataElement* parse(char* data, bool endianSwap = false);

class DataElement
{
public:
    DataElement(bool endianSwap = false) :
        _dataType(miINVALID), _numberOfBytes(0), _isSmallDataElement(false), _endianSwap(endianSwap) {}
    virtual ~DataElement() {}
public:
    EDataType dataType() { return _dataType; }
    uint32_t numberOfBytes() { return _numberOfBytes; }
    bool isSmallDataElement() {return _isSmallDataElement; }
    bool endianSwap() { return _endianSwap; }

    virtual uint32_t totalBytes() { return _isSmallDataElement? 8 : _numberOfBytes + padding() + 8; }
protected:
    virtual uint32_t padding() { return _numberOfBytes % 8? 8 - _numberOfBytes % 8 : 0; }
    virtual void parseTag(char* tag);
protected:
    EDataType _dataType;
    uint32_t _numberOfBytes;
    bool _isSmallDataElement;
    bool _endianSwap;
};

template <typename T>
class FlatDataElement : public DataElement
{
public:
    FlatDataElement(char* data, bool endianSwap = false);
    virtual ~FlatDataElement() {}
public:
    std::vector<T>& data() { return _data; }
protected:
    std::vector<T> _data;
};

class ArrayFlags: public FlatDataElement<uint32_t>
{
public:
    ArrayFlags(char* data, bool endianSwap = false) : FlatDataElement<uint32_t>(data, endianSwap) {}
    virtual ~ArrayFlags() {}
public:
    // NOTE: this differs from that described by offcial document
    bool complex() { return _data[0] & 0x001000; }
    bool global() { return _data[0] & 0x002000; }
    bool logical() { return _data[0] & 0x004000; }
    EMatrixClass klass() { return static_cast<EMatrixClass>(_data[0] & 0xFF); }
    uint32_t nzmax() { return _data[1]; }
};

class DimensionsArray: public FlatDataElement<int32_t>
{
public:
    DimensionsArray(char* data, bool endianSwap = false) : FlatDataElement<int32_t>(data, endianSwap) {}
    virtual ~DimensionsArray() {}
public:
    int32_t dimensionNumber() { return _data.size(); }
    std::vector<int32_t>& dimensions() { return _data; }
};

class Name: public FlatDataElement<int8_t>
{
public:
    Name(char* data, bool endianSwap = false) : FlatDataElement<int8_t>(data, endianSwap), _name() {
        _name.assign(_data.begin(), _data.end());
    }
    virtual ~Name() {}
public:
    std::string name() { return _name; }
protected:
    std::string _name;
};

template <typename T>
class RealImag: public FlatDataElement<T>
{
public:
    RealImag(char* data, bool endianSwap = false) : FlatDataElement<T>(data, endianSwap) {}
    virtual ~RealImag() {}
public:
    uint32_t size() { return this->_data.size(); }
};

class SparseIndex: public FlatDataElement<int32_t>
{
public:
    SparseIndex(char* data, bool endianSwap = false) : FlatDataElement<int32_t>(data, endianSwap) {}
    virtual ~SparseIndex() {}
public:
    std::vector<int32_t>& indices() { return _data; }
};

class FieldNameLength: public FlatDataElement<int32_t>
{
public:
    FieldNameLength(char* data, bool endianSwap = false) : FlatDataElement<int32_t>(data, endianSwap) {}
    virtual ~FieldNameLength() {}
public:
    int32_t fieldNameLength() { return _data[0]; }
};

class FieldNames: public FlatDataElement<int8_t>
{
public:
    FieldNames(char* data, bool endianSwap = false, int32_t fieldNameLength = 32);
    virtual ~FieldNames() {}
public:
    std::vector<std::string> fieldNames() { return _fieldNames; }
    int32_t fieldNameLength() { return _fieldNameLength; }
protected:
    std::vector<std::string> _fieldNames;
    int32_t _fieldNameLength;
};

class CompressedDataElement: public DataElement
{
public:
    CompressedDataElement(char* data, bool endianSwap = false);
    virtual ~CompressedDataElement() { if (_decompressedData) delete[] _decompressedData; }
public:
    char* decompressedData() { return _decompressedData; }
    uint32_t decompressedSize() { return _decompressedSize; }
    DataElement* reparse();
protected:
    char* _decompressedData;
    uint32_t _decompressedSize;
};

// FIXME: UNICODE datatype unimplemented
// unimplemented UNICODE support, but works with ASCII strings
template <EDataType UE>         // UE can be miUTF8, miUTF16, miUTF32
class UnicodeDataElement: public FlatDataElement<unsigned char>
{
public:
    UnicodeDataElement(char* data, bool endianSwap) : FlatDataElement<unsigned char>(data, endianSwap) {}
    virtual ~UnicodeDataElement() {}
public:
    std::string str() { return std::string(_data.begin(), _data.end()); }
};

class MatrixDataElement: public DataElement
{
public:
    MatrixDataElement(bool endianSwap = false) :
        DataElement(endianSwap), _flags(NULL), _dimensionsArray(NULL), _name(NULL) {}
    virtual ~MatrixDataElement() {
        if (_flags) delete _flags;
        if (_dimensionsArray) delete _dimensionsArray;
        if (_name) delete _name;
    }
public:
    ArrayFlags* arrayFlags() { return _flags;}
    DimensionsArray* dimensionsArray() { return _dimensionsArray; }
    Name* name() { return _name; }
protected:
    void parseFlags(char* flags) {
        _flags = new ArrayFlags(flags, _endianSwap);
    };
    void parseDimensionsArray(char* dimensionsArray) {
        _dimensionsArray = new DimensionsArray(dimensionsArray, _endianSwap);
    }
    void parseArrayName(char* arrayName) {
        _name = new Name(arrayName, _endianSwap);
    }
protected:
    ArrayFlags* _flags;
    DimensionsArray* _dimensionsArray;
    Name* _name;
};

template <typename T>
class NumericArray: public MatrixDataElement
{
public:
    NumericArray(char* data, bool endianSwap = false);
    virtual ~NumericArray() {
        if (_real) delete _real;
        if (_imag) delete _imag;
    }
public:
    DataElement* real() { return _real; }
    DataElement* imag() { return _imag; }
protected:
    void parseReal(char* real) { _real = parse(real, _endianSwap); }
    void parseImag(char* imag) { _imag = parse(imag, _endianSwap); }
protected:
    // The real/imag subelement could be RealImag<U> in which U is not T
    // due to automatic compression of numeric data
    DataElement* _real;
    DataElement* _imag;
};

template <typename T>
class SparseArray: public MatrixDataElement
{
public:
    SparseArray(char* data, bool endianSwap = false);
    virtual ~SparseArray() {
        if (_rows) delete _rows;
        if (_cols) delete _cols;
        if (_real) delete _real;
        if (_imag) delete _imag;
    }
public:
    SparseIndex* rows() { return _rows; }
    SparseIndex* cols() { return _cols; }
    DataElement* real() { return _real; }
    DataElement* imag() { return _imag; }
protected:
    void parseRows(char* rows) { _rows = new SparseIndex(rows, _endianSwap); }
    void parseCols(char* cols) { _cols = new SparseIndex(cols, _endianSwap); }
    void parseReal(char* real) { _real = parse(real, _endianSwap); }
    void parseImag(char* imag) { _imag = parse(imag, _endianSwap); }
protected:
    SparseIndex* _rows;
    SparseIndex* _cols;
    // The real/imag subelement could be RealImag<U> in which U is not T
    // due to automatic compression of numeric data
    DataElement* _real;
    DataElement* _imag;
};

class Cell: public MatrixDataElement
{
public:
    Cell(char* data, bool endianSwap = false);
    virtual ~Cell() {
        for (int i = 0; i < _cells.size(); ++i)
            delete _cells[i];
    }
public:
    vector<MatrixDataElement*>& cells() { return _cells; }
protected:
    vector<MatrixDataElement*> _cells;
};

class Struct: public MatrixDataElement
{
public:
    Struct(bool endianSwap = false) : 
        MatrixDataElement(endianSwap), _fieldNameLength(NULL), _fieldNames(NULL), _fields() {}
    Struct(char* data, bool endianSwap = false);
    virtual ~Struct() {
        if (_fieldNameLength) delete _fieldNameLength;
        if (_fieldNames) delete _fieldNames;
        for (int i = 0; i < _fields.size(); ++i)
            delete _fields[i];
    }
public:
    FieldNameLength* fieldNameLength() { return _fieldNameLength; }
    FieldNames* fieldNames() { return _fieldNames; }
    vector<MatrixDataElement*> fields() { return _fields; }
protected:
    void parseFieldNameLength(char* fieldNameLength) {
        _fieldNameLength = new FieldNameLength(fieldNameLength, _endianSwap);
    }
    void parseFieldNames(char* fieldNames) {
        _fieldNames = new FieldNames(fieldNames, _endianSwap, _fieldNameLength->fieldNameLength());
    }
protected:
    FieldNameLength* _fieldNameLength;
    FieldNames* _fieldNames;
    vector<MatrixDataElement*> _fields;
};

class Object: public Struct
{
public:
    Object(char* data, bool endianSwap);
    virtual ~Object() {
        if (_className)
            delete _className;
    }
public:
    Name* className() { return _className; }
protected:
    void parseClassName(char* className) { _className = new Name(className, _endianSwap); }
protected:
    Name* _className;
};



class MatfileReader
{
public:
    MatfileReader(std::ifstream& _inputStream);
    virtual ~MatfileReader();
public:
    std::string descriptiveText() { return _descriptiveText; }
    uint64_t subsysDataOffset() { return _subsysDataOffset; }
    uint16_t version() { return _version; }
    std::string endianIndicator() { return _endianIndicator; }
    int numberDataElements() { return _dataElements.size(); }
    std::vector<DataElement*> dataElements() { return _dataElements; }
public:
    void gotoHeader() { _inputStream.seekg(0, ios_base::beg); }
    void gotoData() { _inputStream.seekg(128, ios_base::beg); }
    void parseHeader();
    bool parseDataElement();
    void parseAllDataElements();
    bool eof() { return _inputStream.eof(); }

private:
    std::string _matfile;
    std::ifstream& _inputStream;

    char _header[128];
    std::string _descriptiveText;
    uint64_t _subsysDataOffset;
    uint16_t _version;
    std::string _endianIndicator;
    bool _endianSwap;
    // infomation of current data element
    std::vector<DataElement*> _dataElements;
};

#endif /* _MATFILEREADER_H_ */
