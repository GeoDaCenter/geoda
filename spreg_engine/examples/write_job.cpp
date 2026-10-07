// Writing a solver job from C++ - the whole wire format, no dependencies.
//
// This is the reference for the GeoDa side of PROTOCOL.md.  It writes the two
// files GeoDa owns (job.json and data.bin) and nothing else; the solver writes
// result.json, result.bin and report.txt back into the same directory.
//
// Build and run:
//     c++ -std=c++17 -o write_job write_job.cpp
//     ./write_job /tmp/geoda-spreg-example ML_Lag
//     "$ENGINE/bin/python3" ../solver/solve.py --job /tmp/geoda-spreg-example
//
// Nothing here needs a JSON library, but GeoDa already carries json_spirit
// (see GdaJson.cpp) and would normally use it to build the spec object; the
// text is assembled by hand here only to keep the example self-contained.
// Reading the answer back is just json_spirit over result.json plus a
// std::ifstream over result.bin - see the README.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace {

// The dtype codes of protocol v1, and their sizes.
const char* kFloat64 = "f8";
const char* kInt64 = "i8";
const char* kInt32 = "i4";

// One entry of the "arrays" table.
struct ArrayRef {
    std::string name;
    std::string dtype;
    std::vector<size_t> shape;
    size_t offset = 0;
    size_t nbytes = 0;
};

// Appends arrays to data.bin and remembers where they landed, exactly the way
// GeoDa would dump the buffers it already has in memory.
class DataBin {
public:
    explicit DataBin(const std::string& path) : out_(path, std::ios::binary) {}

    // Rows are written back to back in row-major order, so a contiguous
    // double[] from the table interface can be written with a single call.
    template <typename T>
    void add(const std::string& name, const std::string& dtype,
             const std::vector<size_t>& shape, const std::vector<T>& values)
    {
        ArrayRef ref;
        ref.name = name;
        ref.dtype = dtype;
        ref.shape = shape;
        ref.offset = offset_;
        ref.nbytes = values.size() * sizeof(T);
        out_.write(reinterpret_cast<const char*>(values.data()),
                   static_cast<std::streamsize>(ref.nbytes));
        offset_ += ref.nbytes;
        arrays_.push_back(ref);
    }

    bool ok() const { return out_.good(); }
    const std::vector<ArrayRef>& arrays() const { return arrays_; }

private:
    std::ofstream out_;
    size_t offset_ = 0;
    std::vector<ArrayRef> arrays_;
};

std::string shape_json(const std::vector<size_t>& shape)
{
    std::ostringstream os;
    os << "[";
    for (size_t i = 0; i < shape.size(); ++i) {
        if (i) os << ", ";
        os << shape[i];
    }
    os << "]";
    return os.str();
}

// job.json: the arrays table is generated from what was actually written.
std::string job_json(const std::string& model, size_t n,
                     const std::vector<std::string>& x_names,
                     const std::vector<ArrayRef>& arrays)
{
    std::ostringstream os;
    os << "{\n"
       << "  \"protocol\": 1,\n"
       << "  \"job_id\": \"cpp-example\",\n"
       << "  \"created\": \"example\",\n"
       << "  \"app\": { \"name\": \"write_job example\", \"version\": \"1\", "
          "\"platform\": \"any\" },\n"
       << "  \"model\": \"" << model << "\",\n"
       << "  \"options\": {},\n"
       << "  \"data\": {\n"
       << "    \"n\": " << n << ",\n"
       << "    \"y\": { \"name\": \"y\", \"array\": \"y\" },\n"
       << "    \"x\": { \"names\": [";
    for (size_t i = 0; i < x_names.size(); ++i) {
        if (i) os << ", ";
        os << "\"" << x_names[i] << "\"";
    }
    os << "], \"array\": \"x\" },\n"
       << "    \"constant\": true,\n"                       // solver adds the intercept
       << "    \"regimes\": { \"name\": \"regime\", \"array\": \"regime\" }\n"
       << "  },\n"
       << "  \"weights\": {\n"
       << "    \"format\": \"csr\",\n"
       << "    \"n\": " << n << ",\n"
       << "    \"indptr\": \"w_indptr\",\n"
       << "    \"indices\": \"w_indices\",\n"
       << "    \"data\": \"w_data\",\n"
       << "    \"transform\": \"r\",\n"                     // row-standardised
       << "    \"name\": \"grid-rook\",\n"
       << "    \"is_symmetric\": true\n"
       << "  },\n"
       << "  \"outputs\": { \"observations\": true, \"report\": true },\n"
       << "  \"arrays\": {\n";
    for (size_t i = 0; i < arrays.size(); ++i) {
        const ArrayRef& a = arrays[i];
        os << "    \"" << a.name << "\": { \"dtype\": \"" << a.dtype
           << "\", \"shape\": " << shape_json(a.shape)
           << ", \"offset\": " << a.offset
           << ", \"nbytes\": " << a.nbytes << " }"
           << (i + 1 == arrays.size() ? "\n" : ",\n");
    }
    os << "  }\n}\n";
    return os.str();
}

}  // namespace

int main(int argc, char** argv)
{
    const std::string dir = argc > 1 ? argv[1] : "geoda-spreg-example";
    const std::string model = argc > 2 ? argv[2] : "OLS";

    std::error_code ec;
    std::filesystem::create_directories(dir, ec);          // GeoDa owns the job dir
    if (ec) {
        std::cerr << "could not create " << dir << ": " << ec.message() << "\n";
        return 1;
    }

    // ---- a small deterministic dataset: a 3 x 4 rook grid ----------------
    const int rows = 3, cols = 4;
    const size_t n = static_cast<size_t>(rows * cols);

    std::vector<double> y(n), x1(n), x2(n), regime(n);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const size_t i = static_cast<size_t>(r * cols + c);
            x1[i] = 0.5 * r + 0.1 * c;
            x2[i] = std::sin(0.7 * static_cast<double>(i));
            regime[i] = c < cols / 2 ? 0.0 : 1.0;
            y[i] = 2.0 + 1.5 * x1[i] - 0.8 * x2[i] + 0.1 * static_cast<double>(i % 3);
        }
    }
    // x is written as one n x 2 block, column by column
    std::vector<double> x;
    x.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) x.push_back(x1[i]);
    for (size_t i = 0; i < n; ++i) x.push_back(x2[i]);

    // ---- the same weights as CSR: GeoDa's GalElement[] in two arrays ----
    std::vector<int64_t> indptr(n + 1, 0);
    std::vector<int32_t> indices;
    std::vector<double> wdata;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const size_t i = static_cast<size_t>(r * cols + c);
            const int dr[4] = {-1, 1, 0, 0};
            const int dc[4] = {0, 0, -1, 1};
            for (int k = 0; k < 4; ++k) {
                const int rr = r + dr[k], cc = c + dc[k];
                if (rr < 0 || rr >= rows || cc < 0 || cc >= cols) continue;
                indices.push_back(static_cast<int32_t>(rr * cols + cc));
                wdata.push_back(1.0);                       // binary contiguity
            }
            indptr[i + 1] = static_cast<int64_t>(indices.size());
        }
    }

    // ---- write data.bin, then job.json ----------------------------------
    const std::string data_path = dir + "/data.bin";
    DataBin bin(data_path);
    bin.add("y", kFloat64, {n, 1}, y);
    bin.add("x", kFloat64, {n, 2}, x);
    bin.add("regime", kFloat64, {n, 1}, regime);
    bin.add("w_indptr", kInt64, {n + 1}, indptr);
    bin.add("w_indices", kInt32, {indices.size()}, indices);
    bin.add("w_data", kFloat64, {wdata.size()}, wdata);
    if (!bin.ok()) {
        std::cerr << "could not write " << data_path << "\n";
        return 1;
    }

    const std::string job_path = dir + "/job.json";
    std::ofstream job(job_path);
    if (!job) {
        std::cerr << "could not write " << job_path << "\n";
        return 1;
    }
    job << job_json(model, n, {"x1", "x2"}, bin.arrays());
    job.close();

    std::cout << "job written to " << dir << " for model " << model << "\n";
    return 0;
}
