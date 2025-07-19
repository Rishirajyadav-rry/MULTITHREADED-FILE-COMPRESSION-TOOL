#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <zlib.h>

std::mutex coutMutex;

bool compressBlock(const std::vector<char>& input, std::vector<char>& output) {
    uLong sourceLen = input.size();
    uLong destLen = compressBound(sourceLen);
    output.resize(destLen);

    int result = compress((Bytef*)output.data(), &destLen, (const Bytef*)input.data(), sourceLen);

    if (result == Z_OK) {
        output.resize(destLen);
        return true;
    }
    return false;
}

void compressFilePart(const std::string& inputFile, const std::string& outputFile, int part, int totalParts) {
    std::ifstream in(inputFile, std::ios::binary | std::ios::ate);
    if (!in) {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cerr << "Failed to open input file.\n";
        return;
    }

    std::streamsize fileSize = in.tellg();
    in.seekg(0, std::ios::beg);

    std::streamsize blockSize = fileSize / totalParts;
    std::streamsize start = part * blockSize;
    std::streamsize end = (part == totalParts - 1) ? fileSize : start + blockSize;

    in.seekg(start);

    std::vector<char> buffer(end - start);
    in.read(buffer.data(), buffer.size());

    std::vector<char> compressed;
    if (!compressBlock(buffer, compressed)) {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cerr << "Compression failed for part " << part << ".\n";
        return;
    }

    std::ofstream out(outputFile + "_part" + std::to_string(part), std::ios::binary);
    out.write(compressed.data(), compressed.size());

    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << "Part " << part << " compressed successfully.\n";
}

int main() {
    std::string inputFile = "sample.txt";     // Your input file
    std::string outputFile = "compressed";    // Output prefix
    int threadCount = 4;

    std::vector<std::thread> threads;

    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back(compressFilePart, inputFile, outputFile, i, threadCount);
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "File compression completed with multithreading.\n";
    return 0;
}
