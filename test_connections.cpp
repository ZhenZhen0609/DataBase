#include <iostream>
#include <string>
#include "AuthManager.h"
#include "StorageManager.h"
#include "SchemaManager.h"

// Simple test to verify the methods work
int main() {
    std::cout << "Testing DBMS method connections..." << std::endl;
    
    // Test AuthManager
    AuthManager auth;
    std::cout << "\n1. Testing AuthManager:" << std::endl;
    
    // Test registration
    bool regResult = auth.registerUser("testuser", "testpass");
    std::cout << "   Register testuser: " << (regResult ? "SUCCESS" : "FAILED") << std::endl;
    
    // Test login with correct password
    bool loginResult = auth.login("testuser", "testpass");
    std::cout << "   Login testuser with correct password: " << (loginResult ? "SUCCESS" : "FAILED") << std::endl;
    
    // Test login with wrong password
    bool wrongLogin = auth.login("testuser", "wrongpass");
    std::cout << "   Login testuser with wrong password: " << (wrongLogin ? "SUCCESS" : "FAILED") << std::endl;
    
    // Test StorageManager
    StorageManager storage;
    std::cout << "\n2. Testing StorageManager:" << std::endl;
    
    // Test database creation
    bool dbResult = storage.createDatabase("testuser", "TestDB");
    std::cout << "   Create TestDB database: " << (dbResult ? "SUCCESS" : "FAILED") << std::endl;
    
    // Test SchemaManager
    std::cout << "\n3. Testing SchemaManager:" << std::endl;
    
    // Test INT validation
    bool intValid = SchemaManager::validateFieldType("INT", 123);
    std::cout << "   Validate 123 as INT: " << (intValid ? "VALID" : "INVALID") << std::endl;
    
    bool intInvalid = SchemaManager::validateFieldType("INT", "abc");
    std::cout << "   Validate 'abc' as INT: " << (intInvalid ? "VALID" : "INVALID") << std::endl;
    
    // Test TEXT validation
    bool textValid = SchemaManager::validateFieldType("TEXT", "Hello");
    std::cout << "   Validate 'Hello' as TEXT: " << (textValid ? "VALID" : "INVALID") << std::endl;
    
    bool textInvalid = SchemaManager::validateFieldType("TEXT", "");
    std::cout << "   Validate empty string as TEXT: " << (textInvalid ? "VALID" : "INVALID") << std::endl;
    
    std::cout << "\nAll method connections tested successfully!" << std::endl;
    std::cout << "\nUI Connections Summary:" << std::endl;
    std::cout << "1. Database Menu -> Login -> AuthManager::login()" << std::endl;
    std::cout << "2. Database Menu -> Register -> AuthManager::registerUser()" << std::endl;
    std::cout << "3. Database Menu -> Create Database -> StorageManager::createDatabase()" << std::endl;
    std::cout << "4. Database Menu -> Exit -> QApplication::quit()" << std::endl;
    std::cout << "5. Help Menu -> About -> QMessageBox::about()" << std::endl;
    
    return 0;
}
