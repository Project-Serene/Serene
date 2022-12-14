// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#pragma once

#include <optional>
#include <string>
#include <functional>
#include <vector>

std::optional<std::string> readFile(const std::string& name);
std::optional<std::string> readStdin();

bool writeFile(const std::string &name,const void *str,int size,unsigned long long count);

bool isDirectory(const std::string& path);
bool traverseDirectory(const std::string& path, const std::function<void(const std::string& name)>& callback);

std::string joinPaths(const std::string& lhs, const std::string& rhs);
std::optional<std::string> getParentPath(const std::string& path);

std::vector<std::string> getSourceFiles(const char *directory);
