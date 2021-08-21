#include "esp_to_text.hpp"
#include "text_to_esp.hpp"

int main() {
    //esp_to_text(L"C:\\Steam\\steamapps\\common\\Skyrim Special Edition\\Data\\empty.esp", L"C:\\test.txt");
    text_to_esp(L"C:\\test.txt", L"C:\\test.esp");
    return 0;
}
