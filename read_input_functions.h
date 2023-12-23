#pragma once

#include <string>
#include <iostream>

std::string ReadLine();

int ReadLineWithNumber();

std::ostream& operator<<(std::ostream& out, const Document& document);
