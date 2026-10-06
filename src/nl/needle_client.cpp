#include "nl/needle_client.h"

#include <fstream>
#include <iterator>

#include <needle.h>

namespace cb {

namespace {

std::string last_error(std::string_view what) {
  const char* err = needle_last_error();
  return std::string(what) + ": " + (err ? err : "unknown error");
}

} // namespace

std::expected<NeedleClient, std::string> NeedleClient::create(
  const std::filesystem::path& model_path,
  const std::string& tools_json,
  const std::string& system_prompt
) {
  std::ifstream in(model_path, std::ios::binary);
  if (!in) return std::unexpected("cannot open model file " + model_path.string());

  NeedleClient client;
  client.weights_.assign(std::istreambuf_iterator<char>(in), {});

  if (needle_load(client.weights_.data(), client.weights_.size()) < 0) {
    return std::unexpected(last_error("needle_load"));
  }
  if (needle_init(system_prompt.c_str(), tools_json.c_str(), nullptr) < 0) {
    return std::unexpected(last_error("needle_init"));
  }
  return client;
}

std::expected<std::string, std::string> NeedleClient::complete(const std::string& input, int max_new_tokens) {
  // Each board command is independent; don't let earlier turns bias the next one.
  needle_reset();

  std::string out(16 * 1024, '\0');
  int n = needle_complete(input.c_str(), nullptr, 0, max_new_tokens, out.data(), static_cast<int>(out.size()));
  if (n < 0) return std::unexpected(last_error("needle_complete"));

  out.resize(out.find('\0'));
  return out;
}

} // namespace cb
