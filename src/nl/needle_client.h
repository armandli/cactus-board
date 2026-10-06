#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace cb {

// RAII wrapper over the Needle 3 C API (needle.h). The engine keeps one
// process-global model, so only one NeedleClient should exist at a time.
class NeedleClient {
public:
  // Loads the .cact weights and initialises the engine with the given tools.
  static std::expected<NeedleClient, std::string> create(
    const std::filesystem::path& model_path,
    const std::string& tools_json,
    const std::string& system_prompt = {}
  );

  NeedleClient(NeedleClient&&) = default;
  NeedleClient& operator=(NeedleClient&&) = default;
  NeedleClient(const NeedleClient&) = delete;
  NeedleClient& operator=(const NeedleClient&) = delete;

  // Runs one stateless turn and returns the engine's raw JSON response.
  std::expected<std::string, std::string> complete(const std::string& input, int max_new_tokens = 512);

private:
  NeedleClient() = default;

  std::vector<unsigned char> weights_; // kept alive; the engine may read it in place
};

} // namespace cb
