#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <mutex>
#include "taffy.h"

namespace Taffy {

// Forward declaration
class StreamingTaffyLoader;

// Handle for streaming TAF files
class StreamingTaffyHandle {
public:
    StreamingTaffyHandle() = default;
    ~StreamingTaffyHandle();
    
    // Move only, no copy
    StreamingTaffyHandle(StreamingTaffyHandle&& other) noexcept;
    StreamingTaffyHandle& operator=(StreamingTaffyHandle&& other) noexcept;
    StreamingTaffyHandle(const StreamingTaffyHandle&) = delete;
    StreamingTaffyHandle& operator=(const StreamingTaffyHandle&) = delete;
    
    bool isValid() const { return loader_ != nullptr; }
    
private:
    friend class StreamingTaffyLoader;
    std::shared_ptr<StreamingTaffyLoader> loader_;
    size_t handle_id_ = 0;
};

// Partial TAF loader for streaming large assets
class StreamingTaffyLoader : public std::enable_shared_from_this<StreamingTaffyLoader> {
    friend class StreamingTaffyHandle;
public:
    StreamingTaffyLoader() = default;
    ~StreamingTaffyLoader();
    
    // Open a TAF file for streaming
    bool open(const std::string& filepath);
    
    // Close the file and release resources
    void close();
    
    // Check if file is open
    bool isOpen() const { return file_.is_open(); }
    
    // Get asset header
    const AssetHeader& getHeader() const { return header_; }
    
    // Get chunk directory
    const std::vector<ChunkDirectoryEntry>& getDirectory() const { return directory_; }
    
    // Load a specific chunk by index
    std::vector<uint8_t> loadChunk(uint32_t index);
    
    // Load a chunk by name
    std::vector<uint8_t> loadChunk(const std::string& name);
    
    // Find chunk index by name
    int findChunkIndex(const std::string& name) const;
    
    // Get chunk info without loading data
    const ChunkDirectoryEntry* getChunkInfo(const std::string& name) const;
    const ChunkDirectoryEntry* getChunkInfo(uint32_t index) const;
    
    // Load only the metadata chunk (first AUDI chunk)
    std::vector<uint8_t> loadMetadata();
    
    // Load audio chunk by sequential index (for streaming audio)
    std::vector<uint8_t> loadAudioChunk(uint32_t chunkIndex);
    
    // Get total number of chunks
    uint32_t getChunkCount() const { return header_.chunk_count; }
    
    // Create a handle for external reference counting
    static StreamingTaffyHandle createHandle(const std::string& filepath);
    
    // Preload specific chunks into cache
    void preloadChunks(const std::vector<uint32_t>& indices);
    
    // Clear chunk cache
    void clearCache();
    
    // Get cache statistics
    struct CacheStats {
        size_t total_chunks_loaded;
        size_t cache_size_bytes;
        size_t cache_hits;
        size_t cache_misses;
    };
    CacheStats getCacheStats() const;
    
private:
    std::string filepath_;
    mutable std::ifstream file_;
    mutable std::mutex file_mutex_;
    AssetHeader header_;
    std::vector<ChunkDirectoryEntry> directory_;
    
    // Simple cache for recently loaded chunks
    struct CachedChunk {
        std::vector<uint8_t> data;
        size_t access_count = 0;
    };
    mutable std::unordered_map<uint32_t, CachedChunk> chunk_cache_;
    mutable std::mutex cache_mutex_;
    mutable size_t cache_hits_ = 0;
    mutable size_t cache_misses_ = 0;
    
    // Handle management
    static std::mutex handle_mutex_;
    static size_t next_handle_id_;
    static std::unordered_map<size_t, std::weak_ptr<StreamingTaffyLoader>> active_handles_;
    
    // Internal chunk loading
    std::vector<uint8_t> loadChunkInternal(uint32_t index) const;
};

// Helper class for creating chunked streaming TAF files
class ChunkedTaffyWriter {
public:
    ChunkedTaffyWriter();
    ~ChunkedTaffyWriter();
    
    // Start writing a new chunked TAF
    bool begin(const std::string& filepath);
    
    // Add metadata chunk (should be first)
    bool addMetadataChunk(const std::vector<uint8_t>& data, const std::string& name = "audio_metadata");
    
    // Add audio data chunk
    bool addAudioChunk(const std::vector<uint8_t>& data, uint32_t chunkIndex);
    
    // Finalize and close the file
    bool finalize();
    
    // Get current chunk count
    uint32_t getChunkCount() const { return chunk_count_; }
    
private:
    std::ofstream file_;
    std::string filepath_;
    std::vector<ChunkDirectoryEntry> directory_;
    uint32_t chunk_count_ = 0;
    size_t current_offset_ = 0;
    bool header_written_ = false;
    
    // Write header and directory
    bool writeHeaderAndDirectory();
};

} // namespace Taffy