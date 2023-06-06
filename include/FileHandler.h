#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

class FileHandler {
public:
  explicit FileHandler(const std::string &filename);
  ~FileHandler();

  std::shared_ptr<std::ofstream> getStream();

private:
  std::shared_ptr<std::ofstream> file;
};

#endif  // FILEHANDLER_H
