
; #######################################
; romanian.nsh
; Romanian language strings for Inkscape installer
; windows code page: 1250
; Authors:
; Translation: Cristian Secar� <cristi AT secarica DOT ro>
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

!insertmacro MUI_LANGUAGE "Romanian"

; Product name
LangString lng_Caption   ${LANG_ENGLISH} "${PRODUCT_NAME} -- Editor Open Source pentru grafic� vectorial� (SVG)"

; Button text "Next >" for the license page
LangString lng_LICENSE_BUTTON   ${LANG_ENGLISH} "�nainte >"

; Bottom text for the license page
LangString lng_LICENSE_BOTTOM_TEXT   ${LANG_ENGLISH} "$(^Name) este publicat sub licen�a public� general� GNU (GPL). Licen�a este furnizat� aici numai cu scop informativ. $_CLICK"

;has been installed by different user
LangString lng_DIFFERENT_USER ${LANG_ENGLISH} "Inkscape a fost deja instalat de utilizatorul $0.$\r$\nDac� ve�i continua, s-ar putea s� nu termina�i instalarea cu succes !$\r$\nAutentifica�i-v� ca $0 �i �ncerca�i din nou."

; want to uninstall before install
LangString lng_WANT_UNINSTALL_BEFORE ${LANG_ENGLISH} "$R1 a fost deja instalat. $\nVre�i s� dezinstala�i versiunea precedent� �nainde de a instala $(^Name) ?"

; press OK to continue press Cancel to abort
LangString lng_OK_CANCEL_DESC ${LANG_ENGLISH} "$\n$\nAp�sa�i butonul OK pentru a continua, sau butonul RENUN�� pentru a opri instalarea."

;you have no admin rigths
LangString lng_NO_ADMIN ${LANG_ENGLISH} "Nu ave�i privilegii de administrator.$\r$\nInstalarea Inkscape pentru to�i utilizatorii ar putea s� nu se termine cu succes.$\r$\nDebifa�i op�iunea �Pentru to�i utilizatorii�."

;win9x is not supported
LangString lng_NOT_SUPPORTED ${LANG_ENGLISH} "Este �tiut c� Inkscape nu ruleaz� sub Windows 95/98/ME !$\r$\nVerifica�i saitul web oficial pentru informa�ii detaliate."

; Full install type
LangString lng_Full $(LANG_ENGLISH) "Complet"

; Optimal install type
LangString lng_Optimal $(LANG_ENGLISH) "Optim"

; Minimal install type
LangString lng_Minimal $(LANG_ENGLISH) "Minim"

; Core install section
LangString lng_Core $(LANG_ENGLISH) "${PRODUCT_NAME} SVG Editor (necesar)"

; Core install section description
LangString lng_CoreDesc $(LANG_ENGLISH) "Fi�iere �i dll-uri indispensabile pentru ${PRODUCT_NAME}"

; GTK+ install section
LangString lng_GTKFiles $(LANG_ENGLISH) "Mediul GTK+ Runtime (necesar)"

; GTK+ install section description
LangString lng_GTKFilesDesc $(LANG_ENGLISH) "Kit de instrumente multiplatform� pentru interfe�e grafice, folosit de ${PRODUCT_NAME}"

; shortcuts install section
LangString lng_Shortcuts $(LANG_ENGLISH) "Scurt�turi"

; shortcuts install section description
LangString lng_ShortcutsDesc $(LANG_ENGLISH) "Scurt�turi pentru pornirea ${PRODUCT_NAME}"

; All user install section
LangString lng_Alluser $(LANG_ENGLISH) "Pentru to�i utilizatorii"

; All user install section description
LangString lng_AlluserDesc $(LANG_ENGLISH) "Instaleaz� aceast� aplica�ie pentru oricine folose�te acest calculator (to�i utilizatorii)"

; Desktop section
LangString lng_Desktop $(LANG_ENGLISH) "Desktop"

; Desktop section description
LangString lng_DesktopDesc $(LANG_ENGLISH) "Creeaz� o scurt�tur� c�tre ${PRODUCT_NAME} pe Desktop"

; Start Menu  section
LangString lng_Startmenu $(LANG_ENGLISH) "Meniul Start"

; Start Menu section description
LangString lng_StartmenuDesc $(LANG_ENGLISH) "Creeaz� o intrare pentru ${PRODUCT_NAME} �n meniul Start"

; Quick launch section
LangString lng_Quicklaunch $(LANG_ENGLISH) "Lansare rapid�"

; Quick launch section description
LangString lng_QuicklaunchDesc $(LANG_ENGLISH) "Creeaz� o scurt�tur� c�tre ${PRODUCT_NAME} pe bara de lansare rapid�"

; File type association for editing
LangString lng_SVGWriter ${LANG_ENGLISH} "Deschidere fi�iere SVG cu ${PRODUCT_NAME}"

; File type association for editing description
LangString lng_SVGWriterDesc ${LANG_ENGLISH} "Selecteaz� ${PRODUCT_NAME} ca editor implicit pentru fi�iere SVG"

; Context Menu
LangString lng_ContextMenu ${LANG_ENGLISH} "Meniu contextual"

; Context Menu description
LangString lng_ContextMenuDesc ${LANG_ENGLISH} "Adaug� ${PRODUCT_NAME} �n meniul contextual pentru fi�iere SVG"

; remove personal preferences
LangString lng_DeletePrefs ${LANG_ENGLISH} "�tergere preferin�ele personale"

; remove personal preferences description
LangString lng_DeletePrefsDesc ${LANG_ENGLISH} "�terge preferin�ele personale r�mase de la instal�ri precedente"


; Additional files section
LangString lng_Addfiles $(LANG_ENGLISH) "Fi�iere adi�ionale"

; Additional files section description
LangString lng_AddfilesDesc $(LANG_ENGLISH) "Fi�iere adi�ionale"

; Examples section
LangString lng_Examples $(LANG_ENGLISH) "Exemple"

; Examples section description
LangString lng_ExamplesDesc $(LANG_ENGLISH) "Exemple folosind ${PRODUCT_NAME}"

; Tutorials section
LangString lng_Tutorials $(LANG_ENGLISH) "Ghiduri practice"

; Tutorials section description
LangString lng_TutorialsDesc $(LANG_ENGLISH) "Ghiduri practice pentru utilizarea ${PRODUCT_NAME}"


; Languages section
LangString lng_Languages $(LANG_ENGLISH) "Traduceri"

; Languages section dscription
LangString lng_LanguagesDesc $(LANG_ENGLISH) "Instaleaz� diverse traduceri ale interfe�ei pentru ${PRODUCT_NAME}"

LangString lng_am $(LANG_ENGLISH) "am  Amharic�"
LangString lng_ar $(LANG_ENGLISH) "ar  Arab�"
LangString lng_az $(LANG_ENGLISH) "az  Azer�"
LangString lng_be $(LANG_ENGLISH) "be  Bielorus�"
LangString lng_bg $(LANG_ENGLISH) "bg  Bulgar�"
LangString lng_bn $(LANG_ENGLISH) "bn  Bengali"
LangString lng_br $(LANG_ENGLISH) "br  Breton�"
LangString lng_ca $(LANG_ENGLISH) "ca  Catalan�"
LangString lng_ca@valencia $(LANG_ENGLISH) "ca@valencia  Catalan�, Valencian"
LangString lng_cs $(LANG_ENGLISH) "cs  Ceh�"
LangString lng_da $(LANG_ENGLISH) "da  Danez�"
LangString lng_de $(LANG_ENGLISH) "de  German�"
LangString lng_dz $(LANG_ENGLISH) "dz  Dzongkha"
LangString lng_el $(LANG_ENGLISH) "el  Greac�"
LangString lng_en $(LANG_ENGLISH) "en  Englez�"
LangString lng_en_AU $(LANG_ENGLISH) "en_AU Englez� australian�"
LangString lng_en_CA $(LANG_ENGLISH) "en_CA Englez� canadian�"
LangString lng_en_GB $(LANG_ENGLISH) "en_GB Englez� britanic�"
LangString lng_en_US@piglatin $(LANG_ENGLISH) "en_US@piglatin Pig Latin"
LangString lng_eo $(LANG_ENGLISH) "eo  Esperanto"
LangString lng_es $(LANG_ENGLISH) "es  Spaniol�"
LangString lng_es_MX $(LANG_ENGLISH) "es_MX  Spaniol� mexican�"
LangString lng_et $(LANG_ENGLISH) "et  Estonian�"
LangString lng_eu $(LANG_ENGLISH) "eu  Basc�"
LangString lng_fi $(LANG_ENGLISH) "fi  Finlandez�"
LangString lng_fr $(LANG_ENGLISH) "fr  Francez�"
LangString lng_ga $(LANG_ENGLISH) "ga  Irlandez�"
LangString lng_gl $(LANG_ENGLISH) "gl  Galician�"
LangString lng_he $(LANG_ENGLISH) "he  Ebraic�"
LangString lng_hr $(LANG_ENGLISH) "hr  Croat�"
LangString lng_hu $(LANG_ENGLISH) "hu  Maghiar�"
LangString lng_id $(LANG_ENGLISH) "id  Indonezian�"
LangString lng_it $(LANG_ENGLISH) "it  Italian�"
LangString lng_ja $(LANG_ENGLISH) "ja  Japonez�"
LangString lng_km $(LANG_ENGLISH) "km  Khmer�"
LangString lng_ko $(LANG_ENGLISH) "ko  Korean�"
LangString lng_lt $(LANG_ENGLISH) "lt  Lituanian�"
LangString lng_mk $(LANG_ENGLISH) "mk  Macedonean�"
LangString lng_mn $(LANG_ENGLISH) "mn  Mongol�"
LangString lng_ne $(LANG_ENGLISH) "ne  Nepalez�"
LangString lng_nb $(LANG_ENGLISH) "nb  Norvegian� c�rtur�reasc�"
LangString lng_nl $(LANG_ENGLISH) "nl  Olandez�"
LangString lng_nn $(LANG_ENGLISH) "nn  Norvegian� nou�"
LangString lng_pa $(LANG_ENGLISH) "pa  Panjabi"
LangString lng_pl $(LANG_ENGLISH) "po  Polonez�"
LangString lng_pt $(LANG_ENGLISH) "pt  Portughez�"
LangString lng_pt_BR $(LANG_ENGLISH) "pt_BR Portughez� brazilian�"
LangString lng_ro $(LANG_ENGLISH) "ro  Roman�n�"
LangString lng_ru $(LANG_ENGLISH) "ru  Rus�"
LangString lng_rw $(LANG_ENGLISH) "rw  Kinyarwanda"
LangString lng_sk $(LANG_ENGLISH) "sk  Slovac�"
LangString lng_sl $(LANG_ENGLISH) "sl  Sloven�"
LangString lng_sq $(LANG_ENGLISH) "sq  Albanez�"
LangString lng_sr $(LANG_ENGLISH) "sr  S�rb�"
LangString lng_sr@latin $(LANG_ENGLISH) "sr@latin  S�rb� (alfabet Latin)"
LangString lng_sv $(LANG_ENGLISH) "sv  Suedez�"
LangString lng_th $(LANG_ENGLISH) "th  Tailandez�"
LangString lng_tr $(LANG_ENGLISH) "tr  Turc�"
LangString lng_uk $(LANG_ENGLISH) "uk  Ucrainean�"
LangString lng_vi $(LANG_ENGLISH) "vi  Vietnamez�"
LangString lng_zh_CN $(LANG_ENGLISH) "zh_CH  Chinez� simplificat�"
LangString lng_zh_TW $(LANG_ENGLISH) "zh_TW  Chinez� tradi�ional�"




; uninstallation options
LangString lng_UInstOpt   ${LANG_ENGLISH} "Op�iuni de dezinstalare"

; uninstallation options subtitle
LangString lng_UInstOpt1  ${LANG_ENGLISH} "Alege�i dintre op�iunile adi�ionale"

; Ask to purge the personal preferences
LangString lng_PurgePrefs ${LANG_ENGLISH} "P�streaz� preferin�ele personale"

LangString lng_RETRY_CANCEL_DESC ${LANG_ENGLISH} "$\n$\nPress RETRY to continue or press CANCEL to abort."

LangString lng_ClearDirectoryBefore ${LANG_ENGLISH} "${PRODUCT_NAME} trebuie s� fie instalat �ntr-un director gol. $INSTDIR nu este gol. Goli�i mai �nt�i acest director !$(lng_RETRY_CANCEL_DESC)"

LangString lng_UninstallLogNotFound ${LANG_ENGLISH} "Fi�ierul $INSTDIR\uninstall.log nu a fost g�sit !$\r$\nDezinstala�i prin golirea manual� a $INSTDIR !"

LangString lng_FileChanged ${LANG_ENGLISH} "Fi�ierul $filename a fost modificat dup� instalare.$\r$\nTot vre�i s� �terge�i acel fi�ier ?"

LangString lng_Yes ${LANG_ENGLISH} "Da"

LangString lng_AlwaysYes ${LANG_ENGLISH} "r�spunde totdeauna cu Da"

LangString lng_No ${LANG_ENGLISH} "Nu"

LangString lng_AlwaysNo ${LANG_ENGLISH} "r�spunde totdeauna cu Nu"
