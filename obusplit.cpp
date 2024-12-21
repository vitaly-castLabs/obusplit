#include <iostream>
#include <fstream>
#include <cstdint>
#include <memory>

// underlying type is set to avoid UB on static_cast under C++17 (it's extracted from a single byte)
enum class ObuType: uint8_t { SeqHdr = 1, TempDelimiter = 2, FrameHdr = 3, TileGroup = 4, Metadata = 5, Frame = 6, RedFrameHdr = 7, TileList = 8, Padding = 15 };

static const char* obuTypeToString[16] = {
    "OBU_RESERVED_0",
    "OBU_SEQUENCE_HEADER",
    "OBU_TEMPORAL_DELIMITER",
    "OBU_FRAME_HEADER",
    "OBU_TILE_GROUP",
    "OBU_METADATA",
    "OBU_FRAME",
    "OBU_REDUNDANT_FRAME_HEADER",
    "OBU_TILE_LIST",
    "OBU_RESERVED_9",
    "OBU_RESERVED_10",
    "OBU_RESERVED_11",
    "OBU_RESERVED_12",
    "OBU_RESERVED_13",
    "OBU_RESERVED_14",
    "OBU_PADDING"
};

void saveFrame(std::vector<std::uint8_t>& frameBuf, int& frameIdx, size_t& totalExtractedFrameSize) {
    if (frameBuf.empty())
        return;

    std::string frameFileName = "frame-" + std::to_string(frameIdx) + ".obu";
    std::ofstream frameFile(frameFileName, std::ios::binary);
    if (!frameFile) {
        std::cerr << "Cannot open " << frameFileName << " \n";
        return;
    }

    frameFile.write(reinterpret_cast<const char*>(frameBuf.data()), frameBuf.size());
    totalExtractedFrameSize += frameBuf.size();

    frameBuf.clear();
    frameIdx++;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "./obusplit input.obu\n";
        return -1;
    }

    std::ifstream obuFile(argv[1], std::ios::binary);
    if (!obuFile) {
        std::cerr << "Cannot open " << argv[1] << " \n";
        return -1;
    }

    obuFile.seekg(0, std::ios::end);
    const auto obuFileSize = obuFile.tellg();
    obuFile.seekg(0, std::ios::beg);

    if (obuFileSize < 2 || obuFileSize > 100'000'000'000) {
        std::cerr << "Unsupported file size (" << obuFileSize << " bytes)\n";
        return -1;
    }

    std::vector<std::uint8_t> obuBuf;
    try {
        obuBuf.resize(obuFileSize + std::streampos(4)); // +4 zeroes to avoid out-of-bounds read
    } catch (const std::bad_alloc& e) {
        std::cerr << "Failed to allocate memory (" << obuFileSize << " bytes)\n";
        return -1;
    }

    if (!obuFile.read(reinterpret_cast<char*>(obuBuf.data()), obuFileSize)) {
        std::cerr << "Failed to read from file\n";
        return -1;
    }

    std::vector<std::uint8_t> frameBuf;
    int frameIdx = 0;
    size_t totalExtractedFrameSize = 0;

    size_t off = 0;
    while (off < obuFileSize) {
        const auto obuStartOff = off;

        const auto obuHdr = obuBuf[off++];
        const auto obuType = ((obuHdr & 0x78) >> 3);
        const auto obuExt = (obuHdr & 0x04) >> 2;
        const auto obuHasSize = (obuHdr & 0x02) >> 1;
        if (!obuHasSize) {
            std::cerr << "obu_has_size_field 0 is not supported\n";
            return -1;
        }

        if (obuExt)
            ++off;

        std::uint64_t obuSize = 0;
        for (size_t i = 0; i < 8; ++i) {
            const auto obuSizeByte = obuBuf[off++];
            obuSize |= (obuSizeByte & 0x7F) << (7 * i);
            if (!(obuSizeByte & 0x80))
                break;
        }
        std::cout << "obu_type: " << obuTypeToString[obuType] << "(" << obuType << "), obu_size: " << obuSize << "\n";

        if (off + obuSize > obuFileSize) {
            std::cerr << "Invalid obu_size / truncated OBU\n";
            break;
        }

        if (obuType == static_cast<int>(ObuType::TempDelimiter))
            saveFrame(frameBuf, frameIdx, totalExtractedFrameSize);

        frameBuf.insert(frameBuf.end(), obuBuf.begin() + obuStartOff, obuBuf.begin() + off + obuSize);
        off += obuSize;
    }

    saveFrame(frameBuf, frameIdx, totalExtractedFrameSize);

    std::cout << "Extracted " << frameIdx << " frames / " << totalExtractedFrameSize << " bytes (input file size: " << obuFileSize << " bytes)\n";

    return 0;
}