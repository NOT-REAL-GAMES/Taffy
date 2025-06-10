#include "include/taffy_streaming.h"
#include <iostream>
#include <cstring>
#include <chrono>

namespace Taffy {

// Static members
std::mutex StreamingTaffyLoader::handle_mutex_;
size_t StreamingTaffyLoader::next_handle_id_ = 1;
std::unordered_map<size_t, std::weak_ptr<StreamingTaffyLoader>> StreamingTaffyLoader::active_handles_;

StreamingTaffyHandle::~StreamingTaffyHandle() {
    if (loader_ && handle_id_ != 0) {
        std::lock_guard<std::mutex> lock(StreamingTaffyLoader::handle_mutex_);
        StreamingTaffyLoader::active_handles_.erase(handle_id_);
    }
}

StreamingTaffyHandle::StreamingTaffyHandle(StreamingTaffyHandle&& other) noexcept
    : loader_(std::move(other.loader_)), handle_id_(other.handle_id_) {
    other.handle_id_ = 0;
}

StreamingTaffyHandle& StreamingTaffyHandle::operator=(StreamingTaffyHandle&& other) noexcept {
    if (this != &other) {
        if (loader_ && handle_id_ != 0) {
            std::lock_guard<std::mutex> lock(StreamingTaffyLoader::handle_mutex_);
            StreamingTaffyLoader::active_handles_.erase(handle_id_);
        }
        loader_ = std::move(other.loader_);
        handle_id_ = other.handle_id_;
        other.handle_id_ = 0;
    }
    return *this;
}

StreamingTaffyLoader::~StreamingTaffyLoader() {
    close();
}

bool StreamingTaffyLoader::open(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    
    if (file_.is_open()) {
        file_.close();
    }
    
    filepath_ = filepath;
    file_.open(filepath_, std::ios::binary);
    if (!file_) {
        std::cerr << "Failed to open TAF file: " << filepath_ << std::endl;
        return false;
    }
    
    // Read header
    file_.read(reinterpret_cast<char*>(&header_), sizeof(header_));
    if (!file_ || file_.gcount() != sizeof(header_)) {
        std::cerr << "Failed to read TAF header" << std::endl;
        file_.close();
        return false;
    }
    
    // Validate magic
    if (std::strncmp(header_.magic, "TAF!", 4) != 0) {
        std::cerr << "Invalid TAF file magic: " << std::string(header_.magic, 4) << std::endl;
        file_.close();
        return false;
    }
    
    // Read chunk directory
    directory_.resize(header_.chunk_count);
    file_.read(reinterpret_cast<char*>(directory_.data()), 
               header_.chunk_count * sizeof(ChunkDirectoryEntry));
    
    if (!file_ || file_.gcount() != header_.chunk_count * sizeof(ChunkDirectoryEntry)) {
        std::cerr << "Failed to read chunk directory" << std::endl;
        file_.close();
        return false;
    }
    
    std::cout << "📖 Opened streaming TAF: " << filepath_ << std::endl;
    std::cout << "   Version: " << header_.version_major << "." 
              << header_.version_minor << "." << header_.version_patch << std::endl;
    std::cout << "   Chunks: " << header_.chunk_count << std::endl;
    std::cout << "   Feature flags: 0x" << std::hex << static_cast<uint64_t>(header_.feature_flags) << std::dec << std::endl;
    
    return true;
}

void StreamingTaffyLoader::close() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    if (file_.is_open()) {
        file_.close();
    }
    directory_.clear();
    clearCache();
}

std::vector<uint8_t> StreamingTaffyLoader::loadChunk(uint32_t index) {
    if (index >= directory_.size()) {
        std::cerr << "Invalid chunk index: " << index << std::endl;
        return {};
    }
    
    // Check cache first
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = chunk_cache_.find(index);
        if (it != chunk_cache_.end()) {
            ++cache_hits_;
            ++it->second.access_count;
            return it->second.data;
        }
        ++cache_misses_;
    }
    
    // Load from file
    auto data = loadChunkInternal(index);
    
    // Add to cache if successful
    if (!data.empty()) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        // Simple cache eviction: remove least accessed chunk if cache is too large
        size_t total_cache_size = 0;
        for (const auto& [idx, chunk] : chunk_cache_) {
            total_cache_size += chunk.data.size();
        }
        
        // If cache is over 50MB, evict least accessed chunk
        while (total_cache_size > 50 * 1024 * 1024 && !chunk_cache_.empty()) {
            auto min_it = chunk_cache_.begin();
            for (auto it = chunk_cache_.begin(); it != chunk_cache_.end(); ++it) {
                if (it->second.access_count < min_it->second.access_count) {
                    min_it = it;
                }
            }
            total_cache_size -= min_it->second.data.size();
            chunk_cache_.erase(min_it);
        }
        
        chunk_cache_[index] = {data, 1};
    }
    
    return data;
}

std::vector<uint8_t> StreamingTaffyLoader::loadChunkInternal(uint32_t index) const {
    std::lock_guard<std::mutex> lock(file_mutex_);
    
    if (!file_.is_open()) {
        std::cerr << "TAF file not open" << std::endl;
        return {};
    }
    
    const auto& entry = directory_[index];
    
    // Seek to chunk data
    file_.seekg(entry.offset);
    if (!file_) {
        std::cerr << "Failed to seek to chunk offset: " << entry.offset << std::endl;
        return {};
    }
    
    // Read chunk data
    std::vector<uint8_t> data(entry.size);
    file_.read(reinterpret_cast<char*>(data.data()), entry.size);
    
    if (!file_ || file_.gcount() != entry.size) {
        std::cerr << "Failed to read chunk data. Expected: " << entry.size 
                  << ", Got: " << file_.gcount() << std::endl;
        return {};
    }
    
    return data;
}

std::vector<uint8_t> StreamingTaffyLoader::loadChunk(const std::string& name) {
    int index = findChunkIndex(name);
    if (index < 0) {
        return {};
    }
    return loadChunk(static_cast<uint32_t>(index));
}

int StreamingTaffyLoader::findChunkIndex(const std::string& name) const {
    for (uint32_t i = 0; i < directory_.size(); ++i) {
        if (std::string(directory_[i].name) == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

const ChunkDirectoryEntry* StreamingTaffyLoader::getChunkInfo(const std::string& name) const {
    for (const auto& entry : directory_) {
        if (std::string(entry.name) == name) {
            return &entry;
        }
    }
    return nullptr;
}

const ChunkDirectoryEntry* StreamingTaffyLoader::getChunkInfo(uint32_t index) const {
    if (index >= directory_.size()) {
        return nullptr;
    }
    return &directory_[index];
}

std::vector<uint8_t> StreamingTaffyLoader::loadMetadata() {
    // Find first AUDI chunk (should be metadata)
    for (uint32_t i = 0; i < directory_.size(); ++i) {
        if (directory_[i].type == ChunkType::AUDI) {
            return loadChunk(i);
        }
    }
    return {};
}

std::vector<uint8_t> StreamingTaffyLoader::loadAudioChunk(uint32_t chunkIndex) {
    std::string chunkName = "audio_chunk_" + std::to_string(chunkIndex);
    return loadChunk(chunkName);
}

StreamingTaffyHandle StreamingTaffyLoader::createHandle(const std::string& filepath) {
    auto loader = std::make_shared<StreamingTaffyLoader>();
    if (!loader->open(filepath)) {
        return StreamingTaffyHandle();
    }
    
    StreamingTaffyHandle handle;
    handle.loader_ = loader;
    
    std::lock_guard<std::mutex> lock(handle_mutex_);
    handle.handle_id_ = next_handle_id_++;
    active_handles_[handle.handle_id_] = loader;
    
    return handle;
}

void StreamingTaffyLoader::preloadChunks(const std::vector<uint32_t>& indices) {
    for (uint32_t index : indices) {
        loadChunk(index); // This will cache the chunk
    }
}

void StreamingTaffyLoader::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    chunk_cache_.clear();
    cache_hits_ = 0;
    cache_misses_ = 0;
}

StreamingTaffyLoader::CacheStats StreamingTaffyLoader::getCacheStats() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    CacheStats stats;
    stats.total_chunks_loaded = chunk_cache_.size();
    stats.cache_size_bytes = 0;
    for (const auto& [idx, chunk] : chunk_cache_) {
        stats.cache_size_bytes += chunk.data.size();
    }
    stats.cache_hits = cache_hits_;
    stats.cache_misses = cache_misses_;
    return stats;
}

// ChunkedTaffyWriter implementation

ChunkedTaffyWriter::ChunkedTaffyWriter() = default;

ChunkedTaffyWriter::~ChunkedTaffyWriter() {
    if (file_.is_open()) {
        finalize();
    }
}

bool ChunkedTaffyWriter::begin(const std::string& filepath) {
    filepath_ = filepath;
    file_.open(filepath_, std::ios::binary);
    if (!file_) {
        std::cerr << "Failed to create TAF file: " << filepath_ << std::endl;
        return false;
    }
    
    // Reserve space for header and directory (we'll write them at the end)
    current_offset_ = sizeof(AssetHeader);
    directory_.clear();
    chunk_count_ = 0;
    header_written_ = false;
    
    return true;
}

bool ChunkedTaffyWriter::addMetadataChunk(const std::vector<uint8_t>& data, const std::string& name) {
    if (!file_.is_open()) {
        std::cerr << "TAF file not open" << std::endl;
        return false;
    }
    
    ChunkDirectoryEntry entry;
    std::strncpy(entry.name, name.c_str(), sizeof(entry.name) - 1);
    entry.name[sizeof(entry.name) - 1] = '\0';
    entry.type = ChunkType::AUDI;
    entry.offset = 0; // Will be set during finalize
    entry.size = data.size();
    entry.flags = 0;
    std::memset(entry.reserved, 0, sizeof(entry.reserved));
    
    directory_.push_back(entry);
    ++chunk_count_;
    
    return true;
}

bool ChunkedTaffyWriter::addAudioChunk(const std::vector<uint8_t>& data, uint32_t chunkIndex) {
    std::string name = "audio_chunk_" + std::to_string(chunkIndex);
    return addMetadataChunk(data, name); // Reuse the same function
}

bool ChunkedTaffyWriter::finalize() {
    if (!file_.is_open() || header_written_) {
        return false;
    }
    
    // Calculate offsets
    current_offset_ = sizeof(AssetHeader) + directory_.size() * sizeof(ChunkDirectoryEntry);
    
    // Update directory with actual offsets
    std::vector<std::vector<uint8_t>> chunk_data;
    
    // First, we need to write header and directory
    AssetHeader header;
    std::strncpy(header.magic, "TAF!", 4);
    header.version_major = 1;
    header.version_minor = 0;
    header.version_patch = 0;
    header.chunk_count = chunk_count_;
    header.asset_type = 0;  // Master asset
    header.feature_flags = static_cast<FeatureFlags>(0x00000002); // FLAG_STREAMING
    header.dependency_count = 0;
    header.ai_model_count = 0;
    header.total_size = 0; // Will calculate
    // Initialize world bounds to zero
    header.world_bounds_min = {0, 0, 0};
    header.world_bounds_max = {0, 0, 0};
    header.created_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    std::memset(header.creator, 0, sizeof(header.creator));
    std::strncpy(header.creator, "ChunkedTaffyWriter", sizeof(header.creator) - 1);
    std::memset(header.description, 0, sizeof(header.description));
    std::strncpy(header.description, "Chunked streaming audio TAF", sizeof(header.description) - 1);
    std::memset(header.reserved, 0, sizeof(header.reserved));
    
    // Update directory offsets
    for (auto& entry : directory_) {
        entry.offset = current_offset_;
        current_offset_ += entry.size;
    }
    
    header.total_size = current_offset_;
    
    // Write header
    file_.seekp(0);
    file_.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // Write directory
    file_.write(reinterpret_cast<const char*>(directory_.data()), 
                directory_.size() * sizeof(ChunkDirectoryEntry));
    
    header_written_ = true;
    file_.close();
    
    std::cout << "✅ Finalized chunked TAF: " << filepath_ << std::endl;
    std::cout << "   Total chunks: " << chunk_count_ << std::endl;
    std::cout << "   Total size: " << header.total_size << " bytes" << std::endl;
    
    return true;
}

} // namespace Taffy