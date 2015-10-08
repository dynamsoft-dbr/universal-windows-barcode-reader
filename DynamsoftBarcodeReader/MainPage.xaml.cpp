//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//
#include <wrl.h>
#include "pch.h"
#include "MainPage.xaml.h"
#include <robuffer.h>

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "If_DBRP.h"

using namespace DynamsoftBarcodeReader;
using namespace Microsoft::WRL;
typedef uint8 byte;

#pragma warning( disable : 4996)
#pragma comment(lib, "DBRx86.lib")

byte* GetPointerToPixelData(IBuffer^ pixelBuffer, unsigned int *length)
{
	if (length != nullptr)
	{
		*length = pixelBuffer->Length;
	}
	// Query the IBufferByteAccess interface.
	ComPtr<IBufferByteAccess> bufferByteAccess;
	reinterpret_cast<IInspectable*>(pixelBuffer)->QueryInterface(IID_PPV_ARGS(&bufferByteAccess));

	// Retrieve the buffer data.
	byte* pixels = nullptr;
	bufferByteAccess->Buffer(&pixels);
	return pixels;
}

struct barcode_format
{
	const char * pszFormat;
	__int64 llFormat;
};

static struct barcode_format Barcode_Formats[] =
{
	{ "CODE_39", CODE_39 },
	{ "CODE_128", CODE_128 },
	{ "CODE_93", CODE_93 },
	{ "CODABAR", CODABAR },
	{ "ITF", ITF },
	{ "UPC_A", UPC_A },
	{ "UPC_E", UPC_E },
	{ "EAN_13", EAN_13 },
	{ "EAN_8", EAN_8 },
	{ "INDUSTRIAL_25",INDUSTRIAL_25 },
	{ "OneD", OneD },
	{ "QR_CODE", QR_CODE }
};

static struct barcode_format Barcode_Formats_Lowcase[] =
{
	{ "code_39", CODE_39 },
	{ "code_128", CODE_128 },
	{ "code_93", CODE_93 },
	{ "codabar", CODABAR },
	{ "itf", ITF },
	{ "upc_a", UPC_A },
	{ "upc_e", UPC_E },
	{ "ean_13", EAN_13 },
	{ "ean_8", EAN_8 },
	{ "industrial_25", INDUSTRIAL_25 },
	{ "oned", OneD },
	{ "qr_code", QR_CODE }
};

__int64 GetFormat(const char * pstr)
{
	__int64 llFormat = 0;
	int iCount = sizeof(Barcode_Formats_Lowcase) / sizeof(Barcode_Formats_Lowcase[0]);

	// convert string to a lowercase string
	int iStrlen = strlen(pstr);
	char * pszFormat = new char[iStrlen + 1];
	memset(pszFormat, 0, iStrlen + 1);
	strcpy(pszFormat, pstr);
	strlwr(pszFormat);

	for (int index = 0; index < iCount; index++)
	{
		if (strstr(pszFormat, Barcode_Formats_Lowcase[index].pszFormat) != NULL)
			llFormat |= Barcode_Formats_Lowcase[index].llFormat;
	}

	delete[] pszFormat;
	return llFormat;
}

const char * GetFormatStr(__int64 format)
{
	int iCount = sizeof(Barcode_Formats) / sizeof(Barcode_Formats[0]);

	for (int index = 0; index < iCount; index++)
	{
		if (Barcode_Formats[index].llFormat == format)
			return Barcode_Formats[index].pszFormat;
	}

	return "UNKNOWN";
}

Array<String^>^ DecodeFile(unsigned char* buffer, int len, int width, int height)
{
	Array<String^>^ results = nullptr;
	String^ barcode_result = nullptr;

	if (!buffer)
		return nullptr;

	// Parse command
	__int64 llFormat = (OneD | QR_CODE);
	int iMaxCount = 0x7FFFFFFF;
	int iIndex = 0;
	ReaderOptions ro = { 0 };
	int iRet = -1;
	char * pszTemp = NULL;
	char * pszTemp1 = NULL;
	unsigned __int64 ullTimeBegin = 0;
	unsigned __int64 ullTimeEnd = 0;

	// combine header info and image data
	char *total = (char *)malloc(len + 40);
	BITMAPINFOHEADER bitmap_info = { 40, width, height, 0, 32, 0, len, 0, 0, 0, 0 };
	memcpy(total, &bitmap_info, 40);
	char *data = total + 40;
	memcpy(data, buffer, len);

	// Set license
	CBarcodeReader reader;
	reader.InitLicense("7B52B86B3A67A4440B1ECB1A536301F4");

	// Read barcode
	ro.llBarcodeFormat = llFormat;
	ro.iMaxBarcodesNumPerPage = iMaxCount;
	reader.SetReaderOptions(ro);
	iRet = reader.DecodeBuffer((unsigned char*)total, len + 40);

	// Output barcode result
	pszTemp = (char*)malloc(4096);
	if (iRet != DBR_OK)
	{
		sprintf(pszTemp, "Failed to read barcode: %s\r\n", DBR_GetErrorString(iRet));
		free(pszTemp);
		return nullptr;
	}

	pBarcodeResultArray paryResult = NULL;
	reader.GetBarcodes(&paryResult);

	if (paryResult->iBarcodeCount == 0)
	{
		sprintf(pszTemp, "No barcode found. Total time spent: %.3f seconds.\r\n", ((float)(ullTimeEnd - ullTimeBegin) / 1000));
		free(pszTemp);
		reader.FreeBarcodeResults(&paryResult);
		return nullptr;
	}

	sprintf(pszTemp, "Total barcode(s) found: %d. Total time spent: %.3f seconds\r\n\r\n", paryResult->iBarcodeCount, ((float)(ullTimeEnd - ullTimeBegin) / 1000));

	results = ref new Array<String^>(paryResult->iBarcodeCount);
	for (iIndex = 0; iIndex < paryResult->iBarcodeCount; iIndex++)
	{
		sprintf(pszTemp, "Barcode %d:\r\n", iIndex + 1);
		sprintf(pszTemp, "%s    Page: %d\r\n", pszTemp, paryResult->ppBarcodes[iIndex]->iPageNum);
		sprintf(pszTemp, "%s    Type: %s\r\n", pszTemp, GetFormatStr(paryResult->ppBarcodes[iIndex]->llFormat));
		pszTemp1 = (char*)malloc(paryResult->ppBarcodes[iIndex]->iBarcodeDataLength + 1);
		memset(pszTemp1, 0, paryResult->ppBarcodes[iIndex]->iBarcodeDataLength + 1);
		memcpy(pszTemp1, paryResult->ppBarcodes[iIndex]->pBarcodeData, paryResult->ppBarcodes[iIndex]->iBarcodeDataLength);
		sprintf(pszTemp, "%s    Value: %s\r\n", pszTemp, pszTemp1);

		// http://stackoverflow.com/questions/11545951/how-to-convert-from-char-to-platformstring-c-cli
		std::string s_str = std::string(pszTemp);
		std::wstring wid_str = std::wstring(s_str.begin(), s_str.end());
		const wchar_t* w_char = wid_str.c_str();
		OutputDebugString(w_char);
		barcode_result = ref new String(w_char);
		results->set(iIndex, barcode_result);
		free(pszTemp1);
		/*sprintf(pszTemp, "    Region: {Left: %d, Top: %d, Width: %d, Height: %d}\r\n\r\n",
		paryResult->ppBarcodes[iIndex]->iLeft, paryResult->ppBarcodes[iIndex]->iTop,
		paryResult->ppBarcodes[iIndex]->iWidth, paryResult->ppBarcodes[iIndex]->iHeight);
		printf(pszTemp);*/
	}

	free(pszTemp);
	reader.FreeBarcodeResults(&paryResult);
	free(total);

	return results;
}

MainPage::MainPage()
{
	InitializeComponent();
}

void MainPage::button_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	FileOpenPicker^ picker = ref new FileOpenPicker();
	picker->FileTypeFilter->Append(".bmp");
	picker->SuggestedStartLocation = PickerLocationId::PicturesLibrary;

	create_task(picker->PickSingleFileAsync()).then([this](StorageFile^ file)
	{
		if (file != nullptr)
		{
			create_task(file->OpenAsync(FileAccessMode::Read)).then([this](IRandomAccessStream^ stream)
			{
				return BitmapDecoder::CreateAsync(stream);
			}).then([this](BitmapDecoder^ decoder) -> IAsyncOperation<SoftwareBitmap^>^
			{
				return decoder->GetSoftwareBitmapAsync();
			}).then([this](SoftwareBitmap^ imageBitmap)
			{
				WriteableBitmap^ wb = ref new WriteableBitmap(imageBitmap->PixelWidth, imageBitmap->PixelHeight);
				imageBitmap->CopyToBuffer(wb->PixelBuffer);
				PreviewImage->Source = wb;

				// xiao: read buffer
				int width = imageBitmap->PixelWidth;
				int height = imageBitmap->PixelHeight;
				unsigned int length;
				unsigned char* data = (unsigned char*)GetPointerToPixelData(wb->PixelBuffer, &length);
				Array<String^>^ results = DecodeFile(data, length, width, height);
				if (results != nullptr && results->Length > 0)
				{
					String^ total_results = nullptr;
					for each (String^ result in results)
					{
						total_results += result + "\n";
					}

					BarcodeResults->Text = total_results;
				}
				else
				{
					BarcodeResults->Text = "no barcode detected";
				}
			});
		}
	});
}
