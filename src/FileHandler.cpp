#include "../include/FileHandler.h"

FileHandler::FileHandler(const std::string &filename) {
  file = std::make_shared<std::ofstream>(filename, std::ios_base::out | std::ios_base::app);
  if (!file->is_open()) {
    throw std::runtime_error("Failed to open the log file");
  }
}

FileHandler::~FileHandler() {
  if (file && file->is_open()) {
    file->close();
  }
}

std::shared_ptr<std::ofstream> FileHandler::getStream() {
  return file;
}
