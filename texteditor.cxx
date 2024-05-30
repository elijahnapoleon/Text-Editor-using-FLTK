
#include <FL/Fl.H>
#include <FL/x.H> // for fl_open_callback
#include <FL/Fl_Group.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/filename.H>

int changed = 0;
char filename[256] = "";
char title[256];
Fl_Text_Buffer* textbuf;

class EditorWindow : public Fl_Double_Window {
public:
	EditorWindow(int w, int h, const char* t);
	~EditorWindow();

	Fl_Window* replace_dlg;
	Fl_Input* replace_find;
	Fl_Input* replace_with;
	Fl_Button* replace_all;
	Fl_Return_Button* replace_next;
	Fl_Button* replace_cancel;

	Fl_Text_Editor* editor;
	char search[256];
};

EditorWindow::EditorWindow(int w, int h, const char* t) : Fl_Double_Window(w,h,t) {
	replace_dlg = new Fl_Window(300, 105, "Replace");
	replace_find = new Fl_Input(70, 10, 200, 25, "Find:");
	replace_with = new Fl_Input(70, 40, 200, 25, "Replace:");
	replace_all = new Fl_Button(10, 70, 90, 25, "Replace All");
	replace_next = new Fl_Return_Button(105, 70, 120, 25, "Replace Next");
	replace_cancel = new Fl_Button(230, 70, 60, 25, "Cancel");

}

EditorWindow::~EditorWindow()
{
	delete replace_dlg;
}

void save_cb(void);

//Checks to see if the current file needs to be saved. If so, asks the user if they want to save it
int check_save(void) {
	if (!changed) return 1;

	int r = fl_choice("The current file has not been saved. \n",
					   "Would you like to save itn now?", "Cancel", "Save", "Discard");

	if (r == 1) {
		save_cb();
		return !changed;
	}

	return(r == 2) ? 1 : 0;

}

//Saves the current buffer to the specified file
void save_file(char* newfile) {
	if (textbuf->savefile(newfile))
		fl_alert("Error writing to file \'%s\':\n%s.", newfile, strerror(errno));
	else
		strcpy(filename, newfile);
	changed = 0;
	textbuf->call_modify_callbacks();
}

//Checks the changed variable and updates the window label accordingly
void set_title(Fl_Window* w) {
	if (filename[0] == '\0') strcpy(title, "Untitled");
	else {
		char* slash;
		slash = strrchr(filename, '/');
#ifndef WIN32
		if (slash == NULL) slash = strrchr(filename, '\\');
#endif // !WIN32
		if (slash != NULL) strcpy(title, slash + 1);
		else strcpy(title, filename);
	}
	if (changed) strcat(title, " (modified");

	w->label(title);
}

//Loads the specified file into the textbuf class
int loading = 0;
void load_file(char* newfile, int ipos) {
	loading = 1;
	int insert = (ipos != -1);
	changed = insert;
	if (!insert) strcpy(filename, "");
	int r;
	if (!insert) r = textbuf->loadfile(newfile);
	else r = textbuf->insertfile(newfile, ipos);
	if (r)
		fl_alert("Error reading from file \'%s\':\n%s.", newfile, strerror(errno));
	else
		if (!insert) strcpy(filename, newfile);
	loading = 0;
	textbuf->call_modify_callbacks();
}

void find_cb(Fl_Widget*, void* v);
void find2_cb(Fl_Widget* w, void* v);
Fl_Window* new_view();


//Called whenever the user changes any text in the editor widget
void changed_cb(int, int nInserted, int nDeleted, int, const char*, void* v) {
	if ((nInserted || nDeleted) && !loading) changed = 1;
	EditorWindow* w = (EditorWindow*)v;
	set_title(w);
	if (loading) w->editor->show_insert_position();
}

//Calls kf_copy() to copy selected text to clipboard
void copy_cb(Fl_Widget*, void* v) {
	EditorWindow* e = (EditorWindow*)v;
	Fl_Text_Editor::kf_cut(0, e->editor);
}

//Call kf_cut() to cut the currently selected text to the clipboard
void cut_cb(Fl_Widget*, void* v)
{
	EditorWindow* e = (EditorWindow*)v;
	Fl_Text_Editor::kf_cut(0, e->editor);
}

//Calls remove_selection() to delete currently selected text
void delete_cb(Fl_Widget*, void* v) {
	textbuf->remove_selection();
}

//Asks for a search string using fl_input() and then calls find2_cb() to find the string
void find_cb(Fl_Widget* w, void* v) {
	EditorWindow* e = (EditorWindow*)v;
	const char* val;
	val = fl_input("Search String:", e->search);
	if (val != NULL) {
		//User enter a string - go find it
		strcpy(e->search, val);
		find2_cb(w, v);
	}
}

//Find the next occurrence of the search string. If search string is blank then pop up the search dialog
void find2_cb(Fl_Widget* w, void* v) {
	EditorWindow* e = (EditorWindow*)v;
	if (e->search[0] == '\0') {
		//Search string is blank; get a new one
		find_cb(w, v);
		return;
	}
	int pos = e->editor->insert_position();
	int found = textbuf->search_forward(pos, e->search, &pos);
	if (found) {
		//Found string;select and update position
		textbuf->select(pos, pos + strlen(e->search));
		e->editor->insert_position(pos + strlen(e->search));
		e->editor->show_insert_position();
	}
	else fl_alert("No occurences of \'%s\' found !", e->search);
}

//Clear the editor widget and current filename. Also calls the check_save function to give the user an opportunity to save the current file first as needed
void new_cb(Fl_Widget*, void*) {
	if (!check_save()) return;

	filename[0] = '\0';
	textbuf->select(0, textbuf->length());
	textbuf->remove_selection();
	changed = 0;
	textbuf->call_modify_callbacks();
}

//Ask the user for a filename then load the specified file into the input widget and current filename. Also calls the check_save function to give the user an opportunity to save the current file first as needed
void open_cb(Fl_Widget*, void*) {
	if (!check_save()) return;

	char* newfile = fl_file_chooser("Open File?", "*", filename);
	if (newfile != NULL) load_file(newfile, -1);
}

//Call kf_paste() to paste the clipboard at the current postion
void paste_cb(Fl_Widget*, void* v) {
	EditorWindow* e = (EditorWindow*)v;
	Fl_Text_Editor::kf_paste(0, e->editor);
}

//See if the current file has been modified, if so give the user a chance to save it. Then exit the program
void quit_cb(Fl_Widget*, void*) {
	if (changed && !check_save()) return;
}

//Shows the replace dialog
void replace_cb(Fl_Widget*, void* v) {
	EditorWindow* e = (EditorWindow*)v;
	e->replace_dlg->show();
}

//Replace the next occurrence of the replacement string. If the replacement string is empty, display the replace dialog
void replace2_cb(Fl_Widget*, void* v) {
	EditorWindow* e = (EditorWindow*)v;
	const char* find = e->replace_find->value();
	const char* replace = e->replace_with->value();

	if (find[0] == '\0') {
		//Replacement string is empty; get a new one
		e->replace_dlg->show();
		return;
	}

	e->replace_dlg->hide();

	int pos = e->editor->insert_position();
	int found = textbuf->search_forward(pos, find, &pos);

	if (found) {
		//Found string; update the position and replace text
		textbuf->select(pos, pos + strlen(find));
		textbuf->remove_selection();
		textbuf->insert(pos, replace);
		textbuf->select(pos, pos + strlen(replace));
		e->editor->insert_position(pos + strlen(replace));
		e->editor->show_insert_position();
	}
	else fl_alert("No occurence of \'%s\' found!", find);
}

//Replace all occurences of replacement string in file
void replall_cb(Fl_Widget*, void* v) {
	EditorWindow* e = (EditorWindow*)v;
	const char* find = e->replace_find->value();
	const char* replace = e->replace_with->value();

	find = e->replace_find->value();
	if (find[0] == '\0') {
		//Replacement string is empty; get a new one
		e->replace_dlg->show();
		return;
	}

	e->replace_dlg->hide();

	e->editor->insert_position(0);
	int times = 0;

	//Loop through whole string
	for (int found = 1; found;) {
		int pos = e->editor->insert_position();
		found = textbuf->search_forward(pos, find, &pos);

		if (found) {
			//Found a match; update postion and replace text
			textbuf->select(pos, pos + strlen(find));
			textbuf->remove_selection();
			textbuf->insert(pos, replace);
			textbuf->select(pos, pos + strlen(replace));
			e->editor->insert_position(pos + strlen(replace));
			e->editor->show_insert_position();
			times++;
		}
	}
	
	if (times) fl_message("Replaced %d occurrences., times");
	else fl_alert("No occurences of \'&s\' found!", find);
}

//Hides the replace dialog
void replcan_cb(Fl_Widget*, void* v) {
	EditorWindow* e = (EditorWindow*)v;
	e->replace_dlg->hide();
}

//Asks the user for a filename and saves the file
void saveas_cb(void) {
	char* newfile;
	
	newfile = fl_file_chooser("Save File As?", "*", filename);
	if (newfile != NULL) save_file(newfile);
}

//Saves the current file. If filename is blank calls saveas_cb()
void save_cb(void) {
	if (filename[0] == '\0') {
		//No filename; get one
		saveas_cb();
		return;
	}
	else save_file(filename);
}

void close_cb(Fl_Widget*, void* v) {
	EditorWindow* w = (EditorWindow*)v;


	w->hide();
	w->editor->buffer(0);
	textbuf->remove_modify_callback(changed_cb, w);
	Fl::delete_widget(w);

}

void view_cb(Fl_Widget*, void*) {
	Fl_Window* w = new_view();
	w->show();
}

Fl_Menu_Item menuitems[] = {
	{"&File", 0, 0, 0, FL_SUBMENU},
		{"&New File", 0, (Fl_Callback*)new_cb},
		{"&Open File...", FL_CTRL + 'o', (Fl_Callback*)open_cb},
		{"&Save File", FL_CTRL + 's', (Fl_Callback*)save_cb},
		{"Save File &As", FL_CTRL + FL_SHIFT + 's',(Fl_Callback*)saveas_cb, 0, FL_MENU_DIVIDER},
		{"New &View", FL_ALT + 'v', (Fl_Callback*)view_cb, 0},
		{"&Close View", FL_CTRL + 'w', (Fl_Callback*)close_cb,0, FL_MENU_DIVIDER},
		{"E&Xit", FL_CTRL + 'q', (Fl_Callback*)quit_cb,0},
		{0},

	{"&Edit", 0, 0, 0, FL_SUBMENU },
		{"Cu&t", FL_CTRL + 'x', (Fl_Callback*)cut_cb},
		{"&Copy", FL_CTRL + 'c', (Fl_Callback*)copy_cb},
		{"&Paste", FL_CTRL + 'v', (Fl_Callback*)paste_cb},
		{"&Delete", 0, (Fl_Callback*)delete_cb},
		{0},

	{"&Search", 0, 0, 0, FL_SUBMENU },
		{"&Find...", FL_CTRL + 'f', (Fl_Callback*)find_cb},
		{"F&ind Again", FL_CTRL + 'g', (Fl_Callback*)find2_cb},
		{"&Replace", FL_CTRL + 'r', (Fl_Callback*)replace_cb},
		{"Re&place Again", FL_CTRL + 't', (Fl_Callback*)replace2_cb},
		{0}


};

Fl_Window* new_view() {
	EditorWindow* w = new EditorWindow(660, 400, title);
	w->begin();
	Fl_Menu_Bar* m = new Fl_Menu_Bar(0, 0, 640, 30);
	m->copy(menuitems,w);
	w->editor = new Fl_Text_Editor(0, 30, 640, 370);
	w->editor->buffer(textbuf);

	textbuf->add_modify_callback(changed_cb, w);
	textbuf->call_modify_callbacks();

	w->end();
	w->resizable(w->editor);
	w->size_range(300, 200);
	w->callback((Fl_Callback*)close_cb, w);

	w->editor->textfont(FL_COURIER);

	return w;
}

int main(int argc, char** argv)
{

	HWND windowHandle = GetConsoleWindow();
	ShowWindow(windowHandle, SW_HIDE);

	textbuf = new Fl_Text_Buffer;

	Fl_Window* window = new_view();

	window->show(1, argv);

	if (argc > 1) load_file(argv[1], -1);

	return Fl::run();
}
