#pragma once

#include <string>
#include <vector>
#include <stdint.h>

std::vector<uint8_t> sha256(const std::string& input);
int hashFileName(const std::string& fileName);