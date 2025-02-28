/** lib includes - 3PP */
#include <fstream>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/detail/error_code.ipp>
#include <boost/range/iterator_range.hpp>
#include <windows.h>
#include <cstdio>

using namespace std;
using namespace boost::filesystem;

void remove_dir_content(const char* a_dir)
{
    remove_all(a_dir);
    create_directory(a_dir);
}

bool str_ends_with_case_insensitive(const string& a_str, const string& a_to_match)
{
    auto it = a_to_match.begin();
    return a_str.size() >= a_to_match.size() && all_of(next(a_str.begin(), a_str.size() - a_to_match.size()), a_str.end(), [&it](const char & c)
    {
        return ::tolower(c) == ::tolower(*(it++));
    });
}

bool is_create_and_run_subprocess(string &a_command, string &a_log_file_name)
{
    bool result = false;

    SECURITY_ATTRIBUTES sec_attrib;
    sec_attrib.nLength = sizeof(sec_attrib);
    sec_attrib.lpSecurityDescriptor = nullptr;
    sec_attrib.bInheritHandle = TRUE;

    HANDLE hlog_file = CreateFile(const_cast<LPCSTR>(a_log_file_name.c_str()),
                                  FILE_APPEND_DATA,
                                  FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
                                  &sec_attrib,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  nullptr );

    STARTUPINFO startup_info;
    PROCESS_INFORMATION process_info;

    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags |= STARTF_USESTDHANDLES;
    startup_info.hStdInput = nullptr;
    startup_info.hStdError = hlog_file;
    startup_info.hStdOutput = hlog_file;
    ZeroMemory(&process_info, sizeof(process_info));

    if( !CreateProcess( nullptr,                                        // Module name
                        const_cast<LPSTR>(a_command.c_str()),        // Command line
                        nullptr,                                        // Process handle not inheritable
                        nullptr,                                        // Thread handle not inheritable
                        TRUE,                                           // Set handle inheritance to FALSE
                        0,                                              // No creation flags
                        nullptr,                                        // Use parent's environment block
                        nullptr,                                        // Use parent's starting directory
                        &startup_info,                                  // Pointer to STARTUPINFO structure
                        &process_info )                                 // Pointer to PROCESS_INFORMATION structure
      )
    {
        printf( "ERROR: CreateProcess failed: %s, error: %lu.\n", a_command.c_str(), GetLastError() );
        return result;
    }

    switch(DWORD return_value = WaitForSingleObject( process_info.hProcess, 3000 ))
    {
    case WAIT_OBJECT_0:
        result = true;
        break;
    case WAIT_ABANDONED:
    case WAIT_TIMEOUT:
    case WAIT_FAILED:
    default:
        printf("ERROR: undefined behavior when execute '%s', error: %lu.\n", a_command.c_str(), return_value);
    }
    CloseHandle(hlog_file);
    TerminateProcess(process_info.hProcess, 0);
    CloseHandle( process_info.hProcess );
    CloseHandle( process_info.hThread );

    if (file_size(a_log_file_name.c_str()) == 0)
        std::remove(a_log_file_name.c_str());

    return result;
}

bool execute_script(const string& a_filename, const string& a_log_file_name)
{
    if (str_ends_with_case_insensitive(a_filename, ".txt"))
    {
        boost::filesystem::path canonicalPath = boost::filesystem::canonical(a_filename);
        string command_line = "\"../../../SigForge/App/SigForge.exe \" ";
        command_line += canonicalPath.string();
        string log_txt = "../OutputFiles/" + a_log_file_name + ".txt";
        //system(command_line.c_str()); /** General system call */
        return is_create_and_run_subprocess(command_line, log_txt); /** Windows specific system call */
    }
    return true;
}

bool execute_script_dir(const char* a_scriptdir)
{
    if (is_directory(path(a_scriptdir)))
        for (auto& lSstack : boost::make_iterator_range(directory_iterator(path(a_scriptdir)), {}))
            if (is_directory(lSstack))
                for (auto& lFile : boost::make_iterator_range(directory_iterator(lSstack), {}))
                    if (is_regular_file(lFile) && !execute_script(lFile.path().string(), lFile.path().stem().string()))
                        return false;
    return true;
}

void compare_dirs(const char* a_dir1, const char* a_dir2, int& a_nr_passes, int& a_nr_fails)
{
    a_nr_passes = 0;
    a_nr_fails = 0;
    if (is_directory(path(a_dir1)))
        for (auto& lFile : boost::make_iterator_range(directory_iterator(a_dir1), {}))
            if (is_regular_file(lFile))
            {
                string l_path = lFile.path().string();
                std::ifstream ifscps(l_path);
                string content((istreambuf_iterator<char>(ifscps)), (istreambuf_iterator<char>()));

                l_path = "/" + l_path.substr(l_path.find_last_of("/\\") + 1);
                l_path = a_dir2 + l_path;

                std::ifstream ifscps2(l_path);
                string content2((istreambuf_iterator<char>(ifscps2)), (istreambuf_iterator<char>()));

                if (content == content2)
                {
                    ++a_nr_passes;
                    cout << "PASS" << endl;
                }
                else
                {
                    ++a_nr_fails;
                    cout << "FAIL: " << l_path.substr(l_path.find_last_of("/\\") + 1) << endl;
                }
            }
}

int main(int argc, char **argv)
{
    remove_dir_content("../OutputFiles");
    if(execute_script_dir("../Resourses"))
    {
        cout << endl << "--------- RESULT ---------" << endl << endl;
        int nr_passes;
        int nr_fails;
        compare_dirs("../OutputTemplateFiles", "../OutputFiles", nr_passes, nr_fails);
        cout << endl << "-------- EO RESULT -------" << endl << endl;
        if (nr_fails)
            cout << endl << nr_passes << " tests passed, " << nr_fails << " tests failed." << endl << endl;
        else
            cout << endl << "[  PASSED  ] " << nr_passes << " tests." << endl << endl;
    }
    cout << endl << "Press ENTER to exit." << endl;
    getchar();
    return 0;
}
