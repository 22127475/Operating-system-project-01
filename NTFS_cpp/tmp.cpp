#include <iostream>
#include <vector>
#include <string>

// Convert vector of bytes to Unicode string (UTF-16)
std::wstring bytesToUnicode(const std::vector<unsigned char>& bytes) {
    // Cast bytes to wchar_t* (UTF-16)
    const wchar_t* wcharData = reinterpret_cast<const wchar_t*>(bytes.data());

    // Construct wstring from wchar_t* data
    return std::wstring(wcharData, bytes.size() / sizeof(wchar_t));
}

int main() {
    // Example byte vector representing UTF-16 encoded string "Hello"
    std::vector<unsigned char> bytes = {0xFF, 0xFE, 0x48, 0x00, 0x65, 0x00, 0x6C, 0x00, 0x6C, 0x00, 0x6F, 0x00};
    // Convert bytes to Unicode string
    std::wstring unicodeStr = bytesToUnicode(bytes);

    // Print or use the Unicode string
    std::wcout << unicodeStr << std::endl;

    return 0;
}
