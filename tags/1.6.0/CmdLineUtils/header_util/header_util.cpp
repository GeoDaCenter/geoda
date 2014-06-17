#include <cstring>
#include <iostream>
#include <vector>
#include <wx/filefn.h> // ::wxFileExists, ::wxCopyFile, etc.
#include <wx/log.h>
#include <wx/string.h>
#include <wx/textfile.h> // wxTextFile
#include <wx/txtstrm.h> // wxTextInputStream
#include <wx/wfstream.h> // wxFileInputStream

using namespace std; // cout, cerr, clog

void display_usage()
{
	cout << "Usage: header_util help\n";
	cout << "       header_util test\n";
	cout << "       header_util delete_lines <file list text file> "
		"<number of lines>\n";
	cout << "       header_util prepend <file list text file> "
		"<prepend text file>\n" << endl;	
}

void get_file_names(const wxString& file_list_fname,
					vector<wxString>& file_names)
{
	wxFileInputStream fl_file_is(file_list_fname);
	wxTextInputStream fl_text_is(fl_file_is);
	wxString line;
	while (!fl_file_is.Eof()) {
		fl_text_is >> line;
		if (!line.IsEmpty()) {
			if (!wxFileExists(line)) {
				wxLogMessage("Warning: " + line + " does not exist");
			} else {
				file_names.push_back(line);
			}
		}
	}
}

void get_prepend_lines(const wxString& prepend_text_fname,
					   vector<wxString>& pt_lines)
{
	wxFileInputStream pt_file_is(prepend_text_fname);
	wxTextInputStream pt_text_is(pt_file_is);
	while (!pt_file_is.Eof()) {
		pt_lines.push_back(pt_text_is.ReadLine());
	}
}

void test()
{
	cout << "Test" << endl;
	if (!wxFileExists("foo_bar.txt")) {
		cout << "foo_bar.txt does not exist." << endl;
	}
	if (!wxFileExists("file_list.txt")) {
		cout << "file_list.txt does not exist, exiting." << endl;
		return;
	}
	if (!wxFileExists("prepend_text.txt")) {
		cout << "prepend_text.txt does not exist, exiting." << endl;
		return;
	}
	
	cout << "Contents of file_list.txt:" << endl;
	vector<wxString> file_names;
	get_file_names("file_list.txt", file_names);
	for (int i=0, iend=file_names.size(); i<iend; i++) {
		wxLogMessage(file_names[i]);
	}
	
	cout << "Contents of prepend_text.txt:" << endl;
	vector<wxString> pt_lines;
	get_prepend_lines("prepend_text.txt", pt_lines);
	for (int i=0, iend=pt_lines.size(); i<iend; i++) {
		wxLogMessage(pt_lines[i]);
	}
}

void delete_lines(const wxString& file_list_fname, long num_lines)
{
	cout << "Deleting " << num_lines << " from top of each file ";
	cout << "listed in " << file_list_fname << endl;
	if (!wxFileExists(file_list_fname)) {
		cout << "Error: " << file_list_fname << " does not exist." << endl;
		return;
	}
	vector<wxString> file_names;
	get_file_names(file_list_fname, file_names);
	
	for (int i=0, iend=file_names.size(); i<iend; i++) {
		wxString msg;
		msg << "Deleting first " << num_lines << " lines: " << file_names[i];
		wxLogMessage(msg);
		wxTextFile tf;
		tf.Open(file_names[i]); // file loaded into memory
		int del_lines = tf.GetLineCount();
		if (del_lines > num_lines) del_lines = num_lines;
		for (long j=0; j<del_lines; j++) tf.RemoveLine(0);
		tf.Write(); // must call write to save changes
	}
}

void prepend(const wxString& file_list_fname,
			 const wxString& prepend_text_fname)
{
	cout << "Prepending text in " << prepend_text_fname << " to top of each ";
	cout << "file listed in " << file_list_fname << endl;
	if (!wxFileExists(file_list_fname)) {
		cout << "Error: " << file_list_fname << " does not exist." << endl;
		return;
	}
	if (!wxFileExists(prepend_text_fname)) {
		cout << "Error: " << prepend_text_fname << " does not exist." << endl;
		return;
	}
	vector<wxString> file_names;
	get_file_names(file_list_fname, file_names);
	vector<wxString> pt_lines;
	get_prepend_lines(prepend_text_fname, pt_lines);
	
	for (int i=0, iend=file_names.size(); i<iend; i++) {
		wxLogMessage("Prepending text to: " + file_names[i]);
		// do not include file name in constructor, unless we want the file
		// overwritten.
		wxTextFile tf;
		tf.Open(file_names[i]); // file loaded into memory
		for (int j=pt_lines.size()-1; j>=0; j--) {
			tf.InsertLine(pt_lines[j], 0);
		}
		tf.Write(); // must call write to save changes
	}
}

int main(int argc, char **argv)
{
	cout << "header_util: A program for modifying the top lines\n";
	cout << "             of source files.\n" << endl;
	wxLog* logger = new wxLogStream(&std::cout);
	wxLog::SetActiveTarget(logger);
	//wxLogMessage("Sample Log Message, works with threads!");
	
	if (argc < 2) {
		cout << "Error: Insufficient number of arguments." << endl;
		display_usage();
	} else if (strcmp(argv[1], "help") == 0) {
		display_usage();
	} else if (strcmp(argv[1], "test") == 0) {
		test();
	} else if (strcmp(argv[1], "delete_lines") == 0) {
		if (argc < 4) {
			cout << "Error: Insufficient number of arguments for delete_lines.";
			cout << endl;
			cout << "Usage: header_util delete_lines <file list text file> "
			"<number of lines>\n";
		} else {
			wxString file_list_fname(argv[2]);
			wxString str_num_lines(argv[3]);
			long num_lines = 0;
			str_num_lines.ToLong(&num_lines);
			if (num_lines < 0) num_lines = 0;
			delete_lines(file_list_fname, num_lines);
		}
	} else if (strcmp(argv[1], "prepend") == 0) {
		if (argc < 4) {
			cout << "Error: Insufficient number of arguments for prepend.";
			cout << endl;
			cout << "Usage: header_util prepend <file list text file> "
			"<prepend text file>\n" << endl;
		} else {
			wxString file_list_fname(argv[2]);
			wxString prepend_text_fname(argv[3]);
			prepend(file_list_fname, prepend_text_fname);
		}
	} else {
		cout << "Error: Unknown command \"" << argv[1] << "\"." << endl;
		display_usage();
	}
	
	delete logger;
	return 0;
}
