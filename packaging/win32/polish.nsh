; #######################################
; polish.nsh
; polish language strings for inkscape installer
; windows code page: 1250
; Authors:
; Przemys�aw Loesch p_loesch@poczta.onet.pl
; Marcin Floryan marcin.floryan@gmail.com - 2008
; Leszek(teo)�yczkowski leszekz@gmail.com - 2009
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
LangString lng_Caption   ${LANG_POLISH} "${PRODUCT_NAME} � otwarte oprogramowanie do grafiki wektorowej SVG"

; Button text "Next >" for the license page
LangString lng_LICENSE_BUTTON   ${LANG_POLISH} "Dalej �"

; Bottom text for the license page
LangString lng_LICENSE_BOTTOM_TEXT   ${LANG_POLISH} "$(^Name) jest udost�pniony na licencji GNU General Public License (GPL). Tekst licencji jest do��czony jedynie w celach informacyjnych. $_CLICK"

;has been installed by different user
LangString lng_DIFFERENT_USER ${LANG_POLISH} "Program Inkscape zosta� zainstalowany przez u�ytkownika $0.$\r$\nJe�li instalacja b�dzie kontynuowana, mo�e zako�czy� si� niepowodzeniem!$\r$\nProsz� zalogowa� si� jako $0 i spr�bowa� ponownie."

; want to uninstall before install
LangString lng_WANT_UNINSTALL_BEFORE ${LANG_POLISH} "$R1 jest ju� zainstalowany. $\nCzy chcesz przed zainstalowaniem programu $(^Name) usun�� jego poprzedni� wersj�?"

; press OK to continue press Cancel to abort
LangString lng_OK_CANCEL_DESC ${LANG_POLISH} "$\n$\nAby kontynuowa� instalacj�, naci�nij przycisk OK, aby przerwa� � Anuluj."

;you have no admin rigths
LangString lng_NO_ADMIN ${LANG_POLISH} "Nie masz uprawnie� administratora.$\r$\nInstalacja programu Inkscape dla wszystkich u�ytkownik�w mo�e zako�czy� si� niepowodzeniem.$\r$\nProsz� wy��czy� opcj� �Dla wszystkich u�ytkownik�w�."

;win9x is not supported
LangString lng_NOT_SUPPORTED ${LANG_POLISH} "Program Inkscape nie dzia�a w systemach Windows 95/98/ME!$\r$\nProsz� zapozna� si� z informacjami na ten temat na oficjalnej stronie internetowej programu."

; Full install type
LangString lng_Full $(LANG_POLISH) "Pe�na"

; Optimal install type
LangString lng_Optimal $(LANG_POLISH) "Optymalna"

; Minimal install type
LangString lng_Minimal $(LANG_POLISH) "Minimalna"

; Core install section
LangString lng_Core $(LANG_POLISH) "${PRODUCT_NAME} Edytor SVG (wymagane)"

; Core install section description
LangString lng_CoreDesc $(LANG_POLISH) "Podstawowe pliki i biblioteki dll dla programu ${PRODUCT_NAME}"

; GTK+ install section
LangString lng_GTKFiles $(LANG_POLISH) "�rodowisko pracy GTK+ (wymagane)"

; GTK+ install section description
LangString lng_GTKFilesDesc $(LANG_POLISH) "Wieloplatformowe �rodowisko graficzne, z kt�rego korzysta ${PRODUCT_NAME}"

; shortcuts install section
LangString lng_Shortcuts $(LANG_POLISH) "Skr�ty"

; shortcuts install section description
LangString lng_ShortcutsDesc $(LANG_POLISH) "Skr�ty do uruchamiania programu ${PRODUCT_NAME}"

; All user install section
LangString lng_Alluser $(LANG_POLISH) "dla wszystkich u�ytkownik�w"

; All user install section description
LangString lng_AlluserDesc $(LANG_POLISH) "Program Inkscape zostanie zainstalowany dla wszystkich u�ytkownik�w tego komputera"

; Desktop section
LangString lng_Desktop $(LANG_POLISH) "Pulpit"

; Desktop section description
LangString lng_DesktopDesc $(LANG_POLISH) "Na pulpicie zostanie utworzony skr�t do uruchamiania programu ${PRODUCT_NAME}"

; Start Menu  section
LangString lng_Startmenu $(LANG_POLISH) "Menu Start"

; Start Menu section description
LangString lng_StartmenuDesc $(LANG_POLISH) "W menu Start zostanie utworzony skr�t do uruchamiania programu ${PRODUCT_NAME}"

; Quick launch section
LangString lng_Quicklaunch $(LANG_POLISH) "Pasek szybkiego uruchamiania"

; Quick launch section description
LangString lng_QuicklaunchDesc $(LANG_POLISH) "Na pasku szybkiego uruchamiania zostanie utworzony skr�t do uruchamiania programu ${PRODUCT_NAME}"

; File type association for editing
LangString lng_SVGWriter ${LANG_POLISH} "Otwieraj pliki SVG za pomoc� programu ${PRODUCT_NAME}"

; File type association for editing description
LangString lng_SVGWriterDesc ${LANG_POLISH} "Program ${PRODUCT_NAME} b�dzie domy�lnym edytorem plik�w SVG"

; Context Menu
LangString lng_ContextMenu ${LANG_POLISH} "Menu kontekstowe"

; Context Menu description
LangString lng_ContextMenuDesc ${LANG_POLISH} "Do systemowego menu kontekstowego zostanie dodany program ${PRODUCT_NAME}"

; remove personal preferences
LangString lng_DeletePrefs ${LANG_POLISH} "Usu� ustawienia u�ytkownika"

; remove personal preferences description
LangString lng_DeletePrefsDesc ${LANG_POLISH} "Zostan� usuni�te ustawienia u�ytkownika pozostawione przez poprzednie instalacje"


; Additional files section
LangString lng_Addfiles $(LANG_POLISH) "Pliki dodatkowe"

; Additional files section description
LangString lng_AddfilesDesc $(LANG_POLISH) "Zostan� dodane wybrane poni�ej dodatkowe pliki"

; Examples section
LangString lng_Examples $(LANG_POLISH) "Przyk�ady"

; Examples section description
LangString lng_ExamplesDesc $(LANG_POLISH) "Przyk�ady u�ycia programu ${PRODUCT_NAME}"

; Tutorials section
LangString lng_Tutorials $(LANG_POLISH) "Poradniki"

; Tutorials section description
LangString lng_TutorialsDesc $(LANG_POLISH) "Poradniki jak korzysta� z programu ${PRODUCT_NAME}"


; Languages section
LangString lng_Languages $(LANG_POLISH) "J�zyki interfejsu"

; Languages section dscription
LangString lng_LanguagesDesc $(LANG_POLISH) "Dost�pne j�zyki interfejsu dla programu ${PRODUCT_NAME}"

LangString lng_am $(LANG_POLISH) "am  Amharski"
LangString lng_ar $(LANG_ENGLISH) "ar  Arabski"
LangString lng_az $(LANG_POLISH) "az  Azerski"
LangString lng_be $(LANG_POLISH) "be  Bia�oruski"
LangString lng_bg $(LANG_POLISH) "bg  Bu�garski"
LangString lng_bn $(LANG_POLISH) "bn  Bengalski"
LangString lng_br $(LANG_ENGLISH) "br  Breto�ski"
LangString lng_ca $(LANG_POLISH) "ca  Katalo�ski"
LangString lng_ca@valencia $(LANG_POLISH) "ca@valencia  Walencki-Katalo�ski"
LangString lng_cs $(LANG_POLISH) "cs  Czeski"
LangString lng_da $(LANG_POLISH) "da  Du�ski"
LangString lng_de $(LANG_POLISH) "de  Niemiecki"
LangString lng_dz $(LANG_POLISH) "dz  Dzongkha"
LangString lng_el $(LANG_POLISH) "el  Grecki"
LangString lng_en $(LANG_POLISH) "en  Angielski"
LangString lng_en_AU $(LANG_POLISH) "en-AU  Angielski-Australijski"
LangString lng_en_CA $(LANG_POLISH) "en-CA  Angielski-Kanadyjski"
LangString lng_en_GB $(LANG_POLISH) "en-GB  Angielski-Brytyjski"
LangString lng_en_US@piglatin $(LANG_POLISH) "en_US@piglatin Pig Latin"
LangString lng_eo $(LANG_POLISH) "eo  Esperanto"
LangString lng_es $(LANG_POLISH) "es  Hiszpa�ski"
LangString lng_es_MX $(LANG_POLISH) "es-MX  Hiszpa�ski-Meksyka�ski"
LangString lng_et $(LANG_POLISH) "et  Esto�ski"
LangString lng_eu $(LANG_POLISH) "eu  Baskijski"
LangString lng_fi $(LANG_POLISH) "fi  Fi�ski"
LangString lng_fr $(LANG_POLISH) "fr  Francuski"
LangString lng_ga $(LANG_POLISH) "ga  Irlandzki"
LangString lng_gl $(LANG_POLISH) "gl  Galicyjski"
LangString lng_he $(LANG_POLISH) "he  Hebrajski"
LangString lng_hr $(LANG_POLISH) "hr  Chorwacki"
LangString lng_hu $(LANG_POLISH) "hu  W�gierski"
LangString lng_id $(LANG_POLISH) "id  Indonezyjski"
LangString lng_it $(LANG_POLISH) "it  W�oski"
LangString lng_ja $(LANG_POLISH) "ja  Japo�ski"
LangString lng_km $(LANG_POLISH) "km  Kmerski"
LangString lng_ko $(LANG_POLISH) "ko  Korea�ski"
LangString lng_lt $(LANG_POLISH) "lt  Litewski"
LangString lng_mk $(LANG_POLISH) "mk  Macedo�ski"
LangString lng_mn $(LANG_POLISH) "mn  Mongolski"
LangString lng_ne $(LANG_POLISH) "ne  Nepali"
LangString lng_nb $(LANG_POLISH) "nb  Norweski Bokm�l"
LangString lng_nl $(LANG_POLISH) "nl  Holenderski"
LangString lng_nn $(LANG_POLISH) "nn  Norweski Nynorsk"
LangString lng_pa $(LANG_POLISH) "pa  Pend�abski"
LangString lng_pl $(LANG_POLISH) "pl  Polski"
LangString lng_pt $(LANG_POLISH) "pt  Portugalski"
LangString lng_pt_BR $(LANG_POLISH) "pt_BR Portugalski-Brazylijski"
LangString lng_ro $(LANG_POLISH) "ro  Rumu�ski"
LangString lng_ru $(LANG_POLISH) "ru  Rosyjski"
LangString lng_rw $(LANG_POLISH) "rw  Ruanda-Rundi "
LangString lng_sk $(LANG_POLISH) "sk  S�owacki"
LangString lng_sl $(LANG_POLISH) "sl  S�owe�ski"
LangString lng_sq $(LANG_POLISH) "sq  Alba�ski"
LangString lng_sr $(LANG_POLISH) "sr  Serbski"
LangString lng_sr@Latn $(LANG_POLISH) "sr@Latn  Serbski skrypt �aci�ski"
LangString lng_sv $(LANG_POLISH) "sv  Szwedzki"
LangString lng_th $(LANG_POLISH) "th  Tajski"
LangString lng_tr $(LANG_POLISH) "tr  Turecki"
LangString lng_uk $(LANG_POLISH) "uk  Ukrai�ski"
LangString lng_vi $(LANG_POLISH) "vi  Wietnamski"
LangString lng_zh_CN $(LANG_POLISH) "zh_CH  Chi�ski uproszczony"
LangString lng_zh_TW $(LANG_POLISH) "zh_TW  Chi�ski tradycyjny"

; uninstallation options
LangString lng_UInstOpt   ${LANG_POLISH} "Opcje dezinstalacji"

; uninstallation options subtitle
LangString lng_UInstOpt1  ${LANG_POLISH} "Dokonaj wyboru spo�r�d dodatkowych opcji"

; Ask to purge the personal preferences
LangString lng_PurgePrefs ${LANG_POLISH} "Zachowaj ustawienia u�ytkownika"

LangString lng_RETRY_CANCEL_DESC ${LANG_POLISH} "$\n$\nAby kontynuowa�, naci�nij przycisk Pon�w pr�b�, aby przerwa� - Anuluj."

LangString lng_ClearDirectoryBefore ${LANG_POLISH} "${PRODUCT_NAME} musi by� instalowany w pustym katalogu. Katalog $INSTDIR nie jest pusty. Prosz� najpierw opr�ni� ten katalog! $(lng_RETRY_CANCEL_DESC)"

LangString lng_UninstallLogNotFound ${LANG_POLISH} "Nie znaleziono $INSTDIR\uninstall.log!$\r$\nProsz� wykona� dezinstalacj� r�cznie poprzez usuni�cie katalogu $INSTDIR!"

LangString lng_FileChanged ${LANG_POLISH} "Plik $filename zosta� zmieniony po zainstalowaniu.$\r$\nCzy nadal chcesz usun�� ten plik?"

LangString lng_Yes ${LANG_POLISH} "Tak"

LangString lng_AlwaysYes ${LANG_POLISH} "Tak dla wszystkich"

LangString lng_No ${LANG_POLISH} "Nie"

LangString lng_AlwaysNo ${LANG_POLISH} "Nie dla wszystkich"
