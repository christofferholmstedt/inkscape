; #######################################
; polish.nsh
; polish language strings for inkscape installer
; windows code page: 1250
; translator:
; Przemys�aw Loesch p_loesch@poczta.onet.pl
; Marcin Floryan marcin.floryan@gmail.com
;
; 27 july 2006 new languages en_CA, en_GB, fi, hr, mn, ne, rw, sq
; 11 august 2006 new languages dz bg
; 24 october 2006 new languages en_US@piglatin, th
; 3rd December 2006 new languages eu km
; 14th December 2006 new lng_DeletePrefs, lng_DeletePrefsDesc, lng_WANT_UNINSTALL_BEFORE and lng_OK_CANCEL_DESC
; february 15 2007 new language bn, en_AU, eo, id, ro
; april 11 2007 new language he
; october 2007 new language ca@valencian
; January 2008 new uninstaller messages
; February 2008 new languages ar, br

!insertmacro MUI_LANGUAGE "Polish"

; Product name
LangString lng_Caption   ${LANG_POLISH} "${PRODUCT_NAME} -- edytor grafiki wektorowej open source"

; Button text "Next >" for the license page
LangString lng_LICENSE_BUTTON   ${LANG_POLISH} "Dalej >"

; Bottom text for the license page
LangString lng_LICENSE_BOTTOM_TEXT   ${LANG_POLISH} "$(^Name) jest udost�pniony na licencji GNU General Public License (GPL). Tekst licencji jest do��czony jedynie w celach informacyjnych. $_CLICK"

;has been installed by different user
LangString lng_DIFFERENT_USER ${LANG_POLISH} "Program Inkscape zosta� zainstalowany przez u�ytkownika $0.$\r$\nJe�li b�dziesz teraz kontynuowa� instalacja mo�e nie zosta� zako�czona pomy�lnie!$\r$\nZaloguj si� prosz� jako u�ytkownik $0 i spr�buj ponownie."

; want to uninstall before install
LangString lng_WANT_UNINSTALL_BEFORE ${LANG_POLISH} "$R1 jest ju� zainstalowany. $\nCzy chcesz usun�� poprzedni� wersj� przed zainstalowaniem $(^Name) ?"

; press OK to continue press Cancel to abort
LangString lng_OK_CANCEL_DESC ${LANG_POLISH} "$\n$\nNaci�nij OK aby kontynuowa� lub ANULUJ aby przerwa�."

;you have no admin rigths
LangString lng_NO_ADMIN ${LANG_POLISH} "Nie masz uprawnie� administratora.$\r$\nInstalacja programu Inkscape dla wszystkich u�ytkownik�w mo�e nie zosta� zako�czon pomy�lnie.$\r$\nWy��cz opcj� 'dla wszystkich u�ytkownik�w'."

;win9x is not supported
LangString lng_NOT_SUPPORTED ${LANG_POLISH} "Program Inkscape nie dzia�a w systemach Windows 95/98/ME!$\r$\nZobacz na szczeg�owe informacje na ten temat na oficjalnej stronie internetowej programu."

; Full install type
LangString lng_Full $(LANG_POLISH) "Pe�na"

; Optimal install type
LangString lng_Optimal $(LANG_POLISH) "Optymalna"

; Minimal install type
LangString lng_Minimal $(LANG_POLISH) "Minimalna"

; Core install section
LangString lng_Core $(LANG_POLISH) "${PRODUCT_NAME} Edytor SVG (wymagane)"

; Core install section description
LangString lng_CoreDesc $(LANG_POLISH) "Podstawowe pliki i sterowniki dll dla ${PRODUCT_NAME} "

; GTK+ install section
LangString lng_GTKFiles $(LANG_POLISH) "�rodowisko pracy GTK+ (wymagane)"

; GTK+ install section description
LangString lng_GTKFilesDesc $(LANG_POLISH) "Wieloplatformowe �rodowisko graficzne, z kt�rego korzysta ${PRODUCT_NAME}"

; shortcuts install section
LangString lng_Shortcuts $(LANG_POLISH) "Skr�ty"

; shortcuts install section description
LangString lng_ShortcutsDesc $(LANG_POLISH) "Skr�ty do uruchamiania ${PRODUCT_NAME}"

; All user install section
LangString lng_Alluser $(LANG_POLISH) "dla wszystkich u�ytkownik�w"

; All user install section description
LangString lng_AlluserDesc $(LANG_POLISH) "Zainstaluj program dla wszystkich u�ytkownik�w komputera"

; Desktop section
LangString lng_Desktop $(LANG_POLISH) "Skr�t na pulpicie"

; Desktop section description
LangString lng_DesktopDesc $(LANG_POLISH) "Utw�rz skr�t na pulpicie do uruchamiania ${PRODUCT_NAME}"

; Start Menu  section
LangString lng_Startmenu $(LANG_POLISH) "Menu Start"

; Start Menu section description
LangString lng_StartmenuDesc $(LANG_POLISH) "Utw�rz skr�t w menu Start do uruchamiania ${PRODUCT_NAME}"

; Quick launch section
LangString lng_Quicklaunch $(LANG_POLISH) "Skr�t na pasku szybkiego uruchamiania"

; Quick launch section description
LangString lng_QuicklaunchDesc $(LANG_POLISH) "Utw�rz skr�t do ${PRODUCT_NAME} na pasku szybkiego uruchamiania"

; File type association for editing
LangString lng_SVGWriter ${LANG_POLISH} "Otwieraj pliki SVG za pomoc� ${PRODUCT_NAME}"

; File type association for editing description
LangString lng_SVGWriterDesc ${LANG_POLISH} "Wybierz ${PRODUCT_NAME} jako domy�lny edytor dla plik�w SVG"

; Context Menu
LangString lng_ContextMenu ${LANG_POLISH} "Menu kontekstowe"

; Context Menu description
LangString lng_ContextMenuDesc ${LANG_POLISH} "Dodaj ${PRODUCT_NAME} do menu kontekstowego dla plik�w SVG"

; remove personal preferences
LangString lng_DeletePrefs ${LANG_POLISH} "Usu� preferencje u�ytkownik�w"

; remove personal preferences description
LangString lng_DeletePrefsDesc ${LANG_POLISH} "Usu� preferencje u�ytkownik�w kt�re pozosta�y z poprzedniej instalacji"


; Additional files section
LangString lng_Addfiles $(LANG_POLISH) "Dodatkowe pliki"

; Additional files section description
LangString lng_AddfilesDesc $(LANG_POLISH) "Dodatkowe pliki"

; Examples section
LangString lng_Examples $(LANG_POLISH) "Przyk�ady"

; Examples section description
LangString lng_ExamplesDesc $(LANG_POLISH) "Przyk�adowe pliki dla ${PRODUCT_NAME}"

; Tutorials section
LangString lng_Tutorials $(LANG_POLISH) "Poradniki"

; Tutorials section description
LangString lng_TutorialsDesc $(LANG_POLISH) "Pliki z poradami jak korzysta� z ${PRODUCT_NAME}"


; Languages section
LangString lng_Languages $(LANG_POLISH) "Wersje j�zykowe"

; Languages section dscription
LangString lng_LanguagesDesc $(LANG_POLISH) "T�umaczenia interfejsu ${PRODUCT_NAME} w wybranych j�zykach"

LangString lng_am $(LANG_POLISH) "am  amharski"
LangString lng_ar $(LANG_POLISH) "ar  arabski"
LangString lng_az $(LANG_POLISH) "az  azerski"
LangString lng_be $(LANG_POLISH) "be  bia�oruski"
LangString lng_bg $(LANG_POLISH) "bg  bu�garski"
LangString lng_bn $(LANG_POLISH) "bn  bengalski"
LangString lng_br $(LANG_POLISH) "br  breto�ski"
LangString lng_ca $(LANG_POLISH) "ca  katalo�ski"
LangString lng_ca@valencia $(LANG_POLISH) "ca@valencia  katalo�ski - dialekt walencki"
LangString lng_cs $(LANG_POLISH) "cs  czeski"
LangString lng_da $(LANG_POLISH) "da  du�ski"
LangString lng_de $(LANG_POLISH) "de  niemiecki"
LangString lng_dz $(LANG_POLISH) "dz  dzongka"
LangString lng_el $(LANG_POLISH) "el  grecki"
LangString lng_en $(LANG_POLISH) "en  angielski"
LangString lng_en_AU $(LANG_POLISH) "en_AU angielski (Australia)"
LangString lng_en_CA $(LANG_POLISH) "en_CA angielski (Kanada)"
LangString lng_en_GB $(LANG_POLISH) "en_GB angielski (Wielka Brytania)"
LangString lng_en_US@piglatin $(LANG_POLISH) "en_US@piglatin angielski (Pig Latin)"
LangString lng_eo $(LANG_POLISH) "eo  esperanto"
LangString lng_es $(LANG_POLISH) "es  hiszpa�ski"
LangString lng_es_MX $(LANG_POLISH) "es_MX  hiszpa�ski (Meksyk)"
LangString lng_et $(LANG_POLISH) "et  esto�ski"
LangString lng_eu $(LANG_POLISH) "eu  baskijski"
LangString lng_fi $(LANG_POLISH) "fi  fi�ski"
LangString lng_fr $(LANG_POLISH) "fr  francuski"
LangString lng_ga $(LANG_POLISH) "ga  irlandzki"
LangString lng_gl $(LANG_POLISH) "gl  galicyjski"
LangString lng_he $(LANG_POLISH) "he  hebrajski"
LangString lng_hr $(LANG_POLISH) "hr  chorwacki"
LangString lng_hu $(LANG_POLISH) "hu  w�gierski"
LangString lng_id $(LANG_POLISH) "id  indonezyjski"
LangString lng_it $(LANG_POLISH) "it  w�oski"
LangString lng_ja $(LANG_POLISH) "ja  japo�ski"
LangString lng_km $(LANG_POLISH) "km  khmerski"
LangString lng_ko $(LANG_POLISH) "ko  korea�ski"
LangString lng_lt $(LANG_POLISH) "lt  litewski"
LangString lng_mk $(LANG_POLISH) "mk  macedo�ski"
LangString lng_mn $(LANG_POLISH) "mn  mongolski"
LangString lng_ne $(LANG_POLISH) "ne  nepalski"
LangString lng_nb $(LANG_POLISH) "nb  norweski Bokm�l"
LangString lng_nl $(LANG_POLISH) "nl  du�ski"
LangString lng_nn $(LANG_POLISH) "nn  norweski Nynorsk"
LangString lng_pa $(LANG_POLISH) "pa  pend�abski"
LangString lng_pl $(LANG_POLISH) "pl  polski"
LangString lng_pt $(LANG_POLISH) "pt  portugalski"
LangString lng_pt_BR $(LANG_POLISH) "pt_BR portugalski (Brazylia)"
LangString lng_ro $(LANG_POLISH) "ro  rumu�ski"
LangString lng_ru $(LANG_POLISH) "ru  rosyjski"
LangString lng_rw $(LANG_POLISH) "rw  ruanda-rundi"
LangString lng_sk $(LANG_POLISH) "sk  s�owacki"
LangString lng_sl $(LANG_POLISH) "sl  s�owe�ski"
LangString lng_sq $(LANG_POLISH) "sq  alba�ski"
LangString lng_sr $(LANG_POLISH) "sr  serbski"
LangString lng_sr@latin $(LANG_POLISH) "sr@latin  serbski (alfabet �aci�ski)"
LangString lng_sv $(LANG_POLISH) "sv  szwedzki"
LangString lng_th $(LANG_POLISH) "th  tajski"
LangString lng_tr $(LANG_POLISH) "tr  turecki"
LangString lng_uk $(LANG_POLISH) "uk  ukrai�ski"
LangString lng_vi $(LANG_POLISH) "vi  wietnamski"
LangString lng_zh_CN $(LANG_POLISH) "zh_CH  chi�ski uproszczony"
LangString lng_zh_TW $(LANG_POLISH) "zh_TW  chi�ski tradycyjny"




; uninstallation options
LangString lng_UInstOpt   ${LANG_POLISH} "Opcje odinstalowania"

; uninstallation options subtitle
LangString lng_UInstOpt1  ${LANG_POLISH} "Dokonaj wyboru spo�r�d dodatkowych opcji"

; Ask to purge the personal preferences
LangString lng_PurgePrefs ${LANG_POLISH} "Zachowaj w systemie preferencje u�ytkownika "

LangString lng_RETRY_CANCEL_DESC ${LANG_POLISH} "$\n$\nNaci�nij PON�W by kontynuowa� lub ANULUJ aby przerwa�."

LangString lng_ClearDirectoryBefore ${LANG_POLISH} "${PRODUCT_NAME} musi by� zainstalowany w pustym folderze. $INSTDIR nie jest pusty. Prosz� usu� jego zawarto��!$(lng_RETRY_CANCEL_DESC)"

LangString lng_UninstallLogNotFound ${LANG_POLISH} "Plik $INSTDIR\uninstall.log nie zosta� znaleziony!$\r$\nOdinstaluj program r�cznie usuwaj�c zawarto�� folderu $INSTDIR!"

LangString lng_FileChanged ${LANG_POLISH} "Plik $filename zosta� zmieniony po zainstalowaniu.$\r$\nCzy na pewno chcesz usun�� ten plik?"

LangString lng_Yes ${LANG_POLISH} "Tak"

LangString lng_AlwaysYes ${LANG_POLISH} "Tak na wszystkie"

LangString lng_No ${LANG_POLISH} "Nie"

LangString lng_AlwaysNo ${LANG_POLISH} "Nie na wszystkie"
