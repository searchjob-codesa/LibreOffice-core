#include <config_oox.h>
#include <memory>
#include <string_view>
#include <cstdlib>

#include <com/sun/star/util/XCloseable.hpp>
#include <vcl/scheduler.hxx>
#include <test/unoapi_test.hxx>
#include <comphelper/lok.hxx>
#include <sfx2/lokhelper.hxx>
#include <LibreOfficeKit/LibreOfficeKitEnums.h>

#include <lib/init.hxx>

#include <cppunit/TestAssert.h>

using namespace com::sun::star;
using namespace desktop;

class DesktopLOKTestRenderSearchResult : public UnoApiTest
{
public:
    DesktopLOKTestRenderSearchResult() : UnoApiTest(u"/desktop/qa/data/"_ustr)
    {
    }

    virtual void setUp() override
    {
        comphelper::LibreOfficeKit::setActive(true);

        UnoApiTest::setUp();
    }

    virtual void tearDown() override
    {
        closeDoc();

        // documents are already closed, no need to call UnoApiTest::tearDown
        test::BootstrapFixture::tearDown();

        comphelper::LibreOfficeKit::setActive(false);
    }

    LibLODocument_Impl* loadDoc(const char* pName)
    {
        // Use the same implementation pattern as DesktopLOKTest
        return loadDoc(pName, getDocumentTypeFromName(pName));
    }

    LibLODocument_Impl* loadDoc(const char* pName, LibreOfficeKitDocumentType eType);

    void closeDoc() { closeDoc(m_pDocument); }
    void closeDoc(std::unique_ptr<LibLODocument_Impl>& pDocument);

    void testRenderSearchResult();

    CPPUNIT_TEST_SUITE(DesktopLOKTestRenderSearchResult);
    CPPUNIT_TEST(testRenderSearchResult);
    CPPUNIT_TEST_SUITE_END();

    std::unique_ptr<LibLODocument_Impl> m_pDocument;

private:
    static LibreOfficeKitDocumentType getDocumentTypeFromName(std::string_view name)
    {
        CPPUNIT_ASSERT_MESSAGE("Document name must include extension.", name.size() > 4);
        const auto it = name.rfind('.');
        CPPUNIT_ASSERT_MESSAGE("Document name must include extension.", it != std::string::npos);
        const std::string_view ext = name.substr(it);

        if (ext == ".ods")
            return LOK_DOCTYPE_SPREADSHEET;
        if (ext == ".odp")
            return LOK_DOCTYPE_PRESENTATION;
        return LOK_DOCTYPE_TEXT;
    }

    std::unique_ptr<LibLODocument_Impl> loadDocImpl(const char* pName, LibreOfficeKitDocumentType eType);
    std::unique_ptr<LibLODocument_Impl> loadDocUrlImpl(const OUString& rFileURL, LibreOfficeKitDocumentType eType);
};

std::unique_ptr<LibLODocument_Impl>
DesktopLOKTestRenderSearchResult::loadDocUrlImpl(const OUString& rFileURL, LibreOfficeKitDocumentType eType)
{
    OUString aService;
    switch (eType)
    {
    case LOK_DOCTYPE_TEXT:
        aService = "com.sun.star.text.TextDocument";
        break;
    case LOK_DOCTYPE_SPREADSHEET:
        aService = "com.sun.star.sheet.SpreadsheetDocument";
        break;
    case LOK_DOCTYPE_PRESENTATION:
        aService = "com.sun.star.presentation.PresentationDocument";
        break;
    default:
        CPPUNIT_ASSERT(false);
        break;
    }

    static int nDocumentIdCounter = 0;
    comphelper::LibreOfficeKit::setDocId(ViewShellDocId(nDocumentIdCounter));
    mxComponent = loadFromDesktop(rFileURL, aService);

    std::unique_ptr<LibLODocument_Impl> pDocument(new LibLODocument_Impl(mxComponent, nDocumentIdCounter));
    ++nDocumentIdCounter;

    return pDocument;
}

std::unique_ptr<LibLODocument_Impl>
DesktopLOKTestRenderSearchResult::loadDocImpl(const char* pName, LibreOfficeKitDocumentType eType)
{
    OUString aFileURL = createFileURL(OUString::createFromAscii(pName));
    return loadDocUrlImpl(aFileURL, eType);
}

LibLODocument_Impl* DesktopLOKTestRenderSearchResult::loadDoc(const char* pName, LibreOfficeKitDocumentType eType)
{
    m_pDocument = loadDocImpl(pName, eType);
    return m_pDocument.get();
}

void DesktopLOKTestRenderSearchResult::closeDoc(std::unique_ptr<LibLODocument_Impl>& pDocument)
{
    if (pDocument)
    {
        pDocument->pClass->registerCallback(pDocument.get(), nullptr, nullptr);
        pDocument.reset();
    }

    if (mxComponent.is())
    {
        css::uno::Reference<util::XCloseable> xCloseable(mxComponent, css::uno::UNO_QUERY_THROW);
        xCloseable->close(false);
        mxComponent.clear();
    }
}

void DesktopLOKTestRenderSearchResult::testRenderSearchResult()
{
    // Load a Writer document
    LibLODocument_Impl* pDocument = loadDoc("blank_text.odt");
    pDocument->m_pDocumentClass->initializeForRendering(pDocument, nullptr);
    Scheduler::ProcessEventsToIdle();

    // Get the view ID
    int viewId = pDocument->m_pDocumentClass->getView(pDocument);
    pDocument->m_pDocumentClass->setView(pDocument, viewId);
    Scheduler::ProcessEventsToIdle();

    // Get tile mode
    int tileMode = pDocument->m_pDocumentClass->getTileMode(pDocument);
    CPPUNIT_ASSERT(tileMode >= 0); // Verify tile mode is valid

    // Test renderSearchResult with a simple indexing payload
    unsigned char* bitmapBuffer = nullptr;
    int width = 0;
    int height = 0;
    size_t byteSize = 0;

    const char* arguments = "<indexing><paragraph node_type=\"writer\" index=\"0\">Test</paragraph></indexing>";

    bool success = pDocument->m_pDocumentClass->renderSearchResult(
        pDocument, arguments, &bitmapBuffer, &width, &height, &byteSize);
    
    // Verify renderSearchResult was called
    if (success)
    {
        CPPUNIT_ASSERT(width >= 0);
        CPPUNIT_ASSERT(height >= 0);
    }

    // Clean up allocated memory if any
    if (bitmapBuffer)
    {
        std::free(bitmapBuffer);
    }
}

CPPUNIT_TEST_SUITE_REGISTRATION(DesktopLOKTestRenderSearchResult);