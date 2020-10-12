# TODO:

- [ ] Change `void printDirectory(const char *Dirname, uint8_t Levels);` to `void printDirectory(const char *directoryName, uint8_t levels);` 
- [ ] Change `void deleteFile(fs::FS &fs, const char *Path);` to `void deleteFile(fs::FS &fs, const char *filePath);`
- [ ] change `void downloadFile(const char *FILE_ADDRESS[], const char *FileName[]);` to `void downloadFile(const char * const fileURL);`
- [ ] Get fileName from the URL.
- [ ] Change `void fileList();` to `void fileList(const char *startPath = "/");`.
