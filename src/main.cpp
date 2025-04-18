#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/dnd.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <array>
#include <wx/filename.h>
#include <fstream>
#include <cstdint>

class MyFrame : public wxFrame {
public:
    MyFrame() : wxFrame(nullptr, wxID_ANY, "Video Converter", wxDefaultPosition, wxSize(600, 350)) {
        wxPanel* panel = new wxPanel(this, wxID_ANY);
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        label = new wxStaticText(panel, wxID_ANY, "Drag a file here", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        mainSizer->Add(label, 0, wxEXPAND | wxALL, 10);

        wxBoxSizer* frameSkipSizer = new wxBoxSizer(wxHORIZONTAL);
        frameSkipSizer->Add(new wxStaticText(panel, wxID_ANY, "Process every n-th frame:"), 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
        frameSkipInput = new wxTextCtrl(panel, wxID_ANY);
        frameSkipSizer->Add(frameSkipInput, 2, wxEXPAND | wxRIGHT, 10);
        frameSkipInput->Bind(wxEVT_TEXT, &MyFrame::OnFrameSkipChanged, this);
        mainSizer->Add(frameSkipSizer, 0, wxEXPAND | wxALL, 5);

        wxBoxSizer* frameMaxSizer = new wxBoxSizer(wxHORIZONTAL);
        frameMaxSizer->Add(new wxStaticText(panel, wxID_ANY, "Max Frames to Process:"), 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
        frameMaxInput = new wxTextCtrl(panel, wxID_ANY);
        frameMaxSizer->Add(frameMaxInput, 2, wxEXPAND | wxRIGHT, 10);
        frameMaxInput->Bind(wxEVT_TEXT, &MyFrame::OnFrameMaxChanged, this);
        mainSizer->Add(frameMaxSizer, 0, wxEXPAND | wxALL, 5);

        frameCountLabel = new wxStaticText(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        mainSizer->Add(frameCountLabel, 0, wxEXPAND | wxALL, 10);

        convertButton = new wxButton(panel, wxID_ANY, "Start Conversion");
        mainSizer->Add(convertButton, 0, wxALIGN_CENTER | wxALL, 10);

        imagePanel = new wxStaticBitmap(panel, wxID_ANY, wxBitmap(512, 128));
        mainSizer->Add(imagePanel, 1, wxEXPAND | wxALL, 10);

        panel->SetSizer(mainSizer);
        panel->Layout();

        wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
        frameSizer->Add(panel, 1, wxEXPAND | wxALL, 10);
        this->SetSizer(frameSizer);
        this->Layout();

        SetDropTarget(new MyDropTarget(this));
        convertButton->Bind(wxEVT_BUTTON, &MyFrame::OnConvertClick, this);
    }

    void UpdateLabel(const wxString& text) {
        this->CallAfter([this, text] { label->SetLabel(text); });
    }

    void ShowFirstFrame(const wxString& videoPath) {
        cv::VideoCapture cap(videoPath.ToStdString());
        if (!cap.isOpened()) {
            wxMessageBox("Error opening video!", "Error", wxOK | wxICON_ERROR);
            return;
        }

        cv::Mat frame;
        if (cap.read(frame)) {
            cv::Mat resizedFrame;
            cv::resize(frame, resizedFrame, cv::Size(512, 128));
            cv::cvtColor(resizedFrame, resizedFrame, cv::COLOR_BGR2RGB);

            wxImage wxImg(resizedFrame.cols, resizedFrame.rows, resizedFrame.data, true);
            wxBitmap bitmap(wxImg);
            this->CallAfter([this, bitmap] {
                imagePanel->SetBitmap(bitmap);
                this->Layout();
            });
        }
    }

   void ShowFrameCount(const wxString& videoPath) {
        cv::VideoCapture cap(videoPath.ToStdString());
        if (!cap.isOpened()) {
            wxMessageBox("Error opening video!", "Error", wxOK | wxICON_ERROR);
            return;
        }
    
        frameCount = static_cast<long>(cap.get(cv::CAP_PROP_FRAME_COUNT));
        if (frameCount <= 0) {
            wxMessageBox("Could not determine frame count.", "Error", wxOK | wxICON_ERROR);
            return;
        }
        double fps = cap.get(cv::CAP_PROP_FPS);
        if (fps <= 0) {
            wxMessageBox("Could not determine frames per second.", "Error", wxOK | wxICON_ERROR);
            return;
        }

        // Benutzerdefinierten Wert für frameSkip beibehalten, falls er bereits gesetzt wurde
        long frameSkip;
        if (!frameSkipInput->GetValue().ToLong(&frameSkip) || frameSkip <= 0) {
            frameSkip = static_cast<int>(std::round(fps / 10.0)); // Standardwert setzen, wenn leer oder ungültig
        }

        timePerFrame = static_cast<int>(1000.0 * frameSkip / fps);
        long frameMax;

        // ⚠️ Hier wird geprüft, ob frameMax manuell gesetzt wurde:
        bool frameMaxSet = frameMaxInput->GetValue().ToLong(&frameMax);

        if (!frameMaxSet || frameMax <= 0) {
            frameMax = frameCount / frameSkip; // Berechnung nur, wenn der Wert nicht manuell geändert wurde
        }

        // ⚠️ Ereignisbehandlung temporär deaktivieren, um Endlosschleifen zu vermeiden
        frameSkipInput->SetEvtHandlerEnabled(false);
        frameMaxInput->SetEvtHandlerEnabled(false);

        frameSkipInput->SetValue(wxString::Format("%d", static_cast<int>(frameSkip)));

        // ⚠️ Nur frameMaxInput setzen, wenn der Benutzer keinen Wert eingegeben hat
        if (!frameMaxSet || frameMax <= 0) {
            frameMaxInput->SetValue(wxString::Format("%d", static_cast<int>(frameMax)));
        }

        frameSkipInput->SetEvtHandlerEnabled(true);
        frameMaxInput->SetEvtHandlerEnabled(true);

    this->CallAfter([this, fps] {
            frameCountLabel->SetLabel(wxString::Format("Total Frames: %d | FPS: %.2f | Time/Frame: %d ms", static_cast<int>(frameCount), fps, timePerFrame));
        });
    }

        
    void OnFrameSkipChanged(wxCommandEvent& event) {
        long frameSkip, frameMax;
    
        // Prüfe, ob frameSkip eine gültige Zahl ist
        if (!frameSkipInput->GetValue().ToLong(&frameSkip) || frameSkip <= 0) {
            return; // Falls ungültig, keine Aktion ausführen
        }
    
        // Falls frameMax noch nicht gesetzt wurde (leer oder 0) oder falls frameMax*frameSkip > frameCount
        if ((!frameMaxInput->GetValue().ToLong(&frameMax) || frameMax <= 0)||(frameMax*frameSkip>frameCount)) {
            if (frameCount > 0) { // Sicherstellen, dass frameCount bekannt ist
                frameMax = frameCount / frameSkip;
                frameMaxInput->SetValue(wxString::Format("%ld", frameMax));
            }
        }

        if (!filePath.IsEmpty()) {
            ShowFrameCount(filePath);
        }

    }

    void OnFrameMaxChanged(wxCommandEvent& event) {
        long frameSkip, frameMax;
        // Prüfe, ob frameSkip eine gültige Zahl ist
        if (!frameMaxInput->GetValue().ToLong(&frameMax) || frameMax <= 0) {
            return; // Falls ungültig, keine Aktion ausführen
        }

        if (frameMax>frameCount) {
            frameMax=frameCount;
            frameMaxInput->SetValue(wxString::Format("%ld", frameMax));
        }

        if (frameSkipInput->GetValue().ToLong(&frameSkip) && frameSkip > 0) {
            if (frameCount/frameSkip<frameMax) {
                frameSkip = frameCount/frameMax;
                frameSkipInput->SetValue(wxString::Format("%ld", frameSkip));
            }
        } else {
            frameSkip = frameCount/frameMax;
            frameSkipInput->SetValue(wxString::Format("%ld", frameSkip));
        }

        if (!filePath.IsEmpty()) {
            ShowFrameCount(filePath);
        }
    }
    
private:
    wxStaticText* label;
    wxStaticText* frameCountLabel;
    wxTextCtrl* frameSkipInput;
    wxTextCtrl* frameMaxInput;
    wxButton* convertButton;
    wxStaticBitmap* imagePanel;
    wxString filePath;

    int timePerFrame;
    long frameCount = 0; // Speichert die Gesamtzahl der Frames

    class MyDropTarget : public wxFileDropTarget {
        public:
            MyDropTarget(MyFrame* frame) : frame(frame) {}
        
            bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) wxOVERRIDE {
                if (!filenames.empty()) {
                    frame->filePath = filenames[0];
                    frame->UpdateLabel("Selected file: " + filenames[0]);
                    frame->ShowFirstFrame(filenames[0]);
                    frame->ShowFrameCount(filenames[0]);
                }
                return true;
            }
        
        private:
            MyFrame* frame;
        };

    void DisplayFrames(long frameSkip, long frameMax) {
        cv::VideoCapture cap(filePath.ToStdString());
        if (!cap.isOpened()) {
            wxMessageBox("Error opening video!", "Error", wxOK | wxICON_ERROR);
            return;
        }

        cv::Mat frame, resizedFrame, enlargedFrame;
        std::array<std::array<std::array<uint8_t, 64>, 256>, 3> bmp;            // original x,y bitmap
        std::array<std::array<std::array<uint8_t, 64>, 256>, 3> bmpSorted;      // pixels sorted accorfing to LED arrangement
        std::array<std::array<std::array<uint8_t, 8>, 1792>, 3> bmpLED;         // color encoded with 3 bits in 7 successive columns, 8 bits address 8 LEDs each

        long currentFrame = 0;
        for (long i = 0; i < frameMax; ++i) {
            cap.set(cv::CAP_PROP_POS_FRAMES, currentFrame);
            if (!cap.read(frame)) {
                break;
            }

            std::cout << "Displaying Frame: " << currentFrame << std::endl;
//            UpdateLabel(wxString::Format("Displaying Frame: %ld", currentFrame));

            cv::resize(frame, resizedFrame, cv::Size(256, 64));
            cv::resize(resizedFrame, enlargedFrame, cv::Size(512, 128), 0, 0, cv::INTER_NEAREST);
            cv::cvtColor(resizedFrame, resizedFrame, cv::COLOR_BGR2RGB);

            // Fülle das bmp-Array mit den Farbwerten (Korrektur der Farbkanäle)
            for (int y = 0; y < 64; ++y) {
                for (int x = 0; x < 256; ++x) {
                    cv::Vec3b color = resizedFrame.at<cv::Vec3b>(y, x);
                    bmp[0][x][y] = color[0]; // R 
                    bmp[1][x][y] = color[1]; // G 
                    bmp[2][x][y] = color[2]; // B 
                }
            }
            // Sortiere die Pixel so um, dass sie mit dem rotierenden Dispülay angezeigt werdne können
            // y gerade:                          ; x=0, y=0 ==> xSorted=128, ySorted=32 
            // xSorted = (x+128)%256
            // ySorted = ((int)(y/2))+32          ; ySorted = 32, 33, 34, 35, ... für y = 0, 2, 4, 6, ...
            //
            // y ungerade:
            // xSorted = x
            // ySorted = (int)((y-1)/2)           ; YSorted = 0, 1, 2, 3, 4, ... für y = 1, 3, 5, 7, ...

            for (int y=0 ; y<64; ++y) {
              for (int x = 0; x<256; ++x) {
                int xSorted, ySorted;
                if ((y%2)==0) {                 // y even
                    xSorted = (x+128)%256;      // add 128, fix roll over    
                    ySorted = ((int)(y/2))+32;  // y=0,2,4,6,... => LED=32,33,34,35, ...
                } else {                        // y odd
                    xSorted = x;
                    ySorted = (int)((y-1)/2);   // y=1,3,5,7,... => LED=0,1,2,3
                }
//                wxLogMessage("x: %d, y: %d, xs: %d, ys: %d, R: %d, G: %d, B: %d", 
//                x, y, xSorted, ySorted, bmp[0][x][y], bmp[1][x][y], bmp[2][x][y]);    

                for (int c = 0; c<3; ++c) {
                    bmpSorted[c][xSorted][ySorted] = bmp[c][x][y];
                }
              }
            }  
            
            int pattern[8][7]= {        // 8 intensity levels (0,...,7), 7 successive sub-pixels
                {0,0,0,0,0,0,0},        // 0
                {0,0,0,1,0,0,0},        // 1
                {0,0,1,0,1,0,0},        // 2
                {0,1,0,1,0,1,0},        // 3
                {0,1,1,0,1,1,0},        // 4
                {0,1,1,1,1,1,0},        // 5
                {1,1,1,0,1,1,1},        // 6
                {1,1,1,1,1,1,1}         // 7
            };    

            for (int y=0; y<8; ++y) {          
                for (int x=0; x<256; ++x) {
                    for (int subpixel = 0; subpixel<7; subpixel++) {
                        for (int c=0; c<3; ++c) {
                            uint8_t byte=0;
                            int b=0;
                            int LED=0;
                            for (int bit=0; bit<8; ++bit) {
                                LED = 8*y+7-bit;
                                b = bmpSorted[c][255-x][63-LED]/32;        // reduce color depth from 256 to 8 levels
                                byte |= (uint8_t)(pattern[b][subpixel]<<bit);      // assemble pattern for 8 LEDs 
                            }
                            bmpLED[c][x*7+7-subpixel][y]=byte;            // save to bmpLED
                        }
                    }
                }
            }

            /*
            for (int y=0; y<8; y++) {
                for (int x=0; x<30; x++) {
                    for (int c=0; c<3; c++) {
                        wxLogMessage("x: %d, y: %d, c: %d, value: %d", 
                        x, y, c, bmpLED[c][x][y]); 
                    }
                }
            }
            */
            
            // Speicherort des Videos verwenden
            wxFileName fileName(filePath);
            wxString outputFilePath = fileName.GetPathWithSep() + fileName.GetName() + ".RS64";

            // Datei zum Schreiben öffnen
            std::ofstream outputFile;
            if (currentFrame == 0) {
                outputFile.open(outputFilePath.ToStdString(), std::ios::binary); // Erste Dateiöffnung im Schreibmodus
                int32_t frameMaxInt = static_cast<int32_t>(frameMax);
                int32_t timePerFrameInt = static_cast<int32_t>(timePerFrame);

                // Speichere die Werte als 4-Byte-Integer
                outputFile.write(reinterpret_cast<const char*>(&frameMaxInt), sizeof(frameMaxInt));
                outputFile.write(reinterpret_cast<const char*>(&timePerFrameInt), sizeof(timePerFrameInt));
            } else {
                outputFile.open(outputFilePath.ToStdString(), std::ios::binary | std::ios::app); // Danach im Append-Modus
            }
            if (!outputFile) {
                wxMessageBox("Error creating file!", "Error", wxOK | wxICON_ERROR);
                return;
            }

            // bmpLED-Daten in Datei speichern
            for (int y = 0; y < 8; ++y) {
                for (int x = 0; x < 256; ++x) {
                    for (int subpixel = 0; subpixel < 7; subpixel++) {
                        for (int c = 0; c < 3; ++c) {
                            outputFile.write(reinterpret_cast<const char*>(&bmpLED[c][x * 7 + subpixel][y]), sizeof(uint8_t));
                        }
                    }
                }
            }
            

            // Datei erst nach allen Frames schließen
        if (i == frameMax - 1) {
            outputFile.close();
        }
            if (i == frameMax - 1) {
                wxMessageBox("File saved: " + outputFilePath, "Success", wxOK | wxICON_INFORMATION);
            }

        
            cv::Mat rgbFrame;
            cv::cvtColor(enlargedFrame, rgbFrame, cv::COLOR_BGR2RGB);
            wxImage wxImg(rgbFrame.cols, rgbFrame.rows, rgbFrame.data, true);
            wxBitmap bitmap(wxImg);
            this->CallAfter([this, bitmap] {
                imagePanel->SetBitmap(bitmap);
                this->Layout();
            });
//            wxYield();

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            currentFrame += frameSkip;
        }
    }

    void OnConvertClick(wxCommandEvent& event) {
        if (filePath.IsEmpty()) {
            wxMessageBox("Please select a file first!", "Error", wxOK | wxICON_ERROR);
            return;
        }

        long frameSkip, frameMax;
        if (!frameSkipInput->GetValue().ToLong(&frameSkip) || frameSkip <= 0) {
            wxMessageBox("Invalid value for 'Process every n-th frame'!", "Error", wxOK | wxICON_ERROR);
            return;
        }
        if (!frameMaxInput->GetValue().ToLong(&frameMax) || frameMax <= 0) {
            wxMessageBox("Invalid value for 'Max Frames to Process'!", "Error", wxOK | wxICON_ERROR);
            return;
        }

        std::thread displayThread(&MyFrame::DisplayFrames, this, frameSkip, frameMax);
        displayThread.detach();
    }
};

class MyApp : public wxApp {
public:
    bool OnInit() override {
        MyFrame* frame = new MyFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);