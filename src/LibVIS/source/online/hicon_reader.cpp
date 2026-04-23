#include <hicon_reader.h>
typedef const char* LPCSTR;
using namespace std::chrono;

std::string GetDeviceName(MV_CC_DEVICE_INFO* pDeviceInfo) {
    std::string res;
    std::string UDN;
    std::string MN;
    std::string SN;

    if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
    {
        UDN = std::string((LPCSTR)(pDeviceInfo->SpecialInfo.stGigEInfo.chUserDefinedName));
        MN = std::string((LPCSTR)(pDeviceInfo->SpecialInfo.stGigEInfo.chModelName));
        SN = std::string((LPCSTR)(pDeviceInfo->SpecialInfo.stGigEInfo.chSerialNumber));
    }
    else if (pDeviceInfo->nTLayerType == MV_USB_DEVICE)
    {
        UDN = std::string((LPCSTR)(pDeviceInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName));
        MN = std::string((LPCSTR)(pDeviceInfo->SpecialInfo.stUsb3VInfo.chModelName));
        SN = std::string((LPCSTR)(pDeviceInfo->SpecialInfo.stUsb3VInfo.chSerialNumber));
    }
    else if (pDeviceInfo->nTLayerType == MV_GENTL_CAMERALINK_DEVICE)
    {
        UDN = std::string((LPCSTR)(pDeviceInfo->SpecialInfo.stCMLInfo.chUserDefinedName));
        MN = std::string((LPCSTR)(pDeviceInfo->SpecialInfo.stCMLInfo.chModelName));
        SN = std::string((LPCSTR)(pDeviceInfo->SpecialInfo.stCMLInfo.chSerialNumber));
    }
    else if (pDeviceInfo->nTLayerType == MV_GENTL_CXP_DEVICE)
    {
        UDN = std::string((LPCSTR)(pDeviceInfo->SpecialInfo.stCXPInfo.chUserDefinedName));
        MN = std::string((LPCSTR)(pDeviceInfo->SpecialInfo.stCXPInfo.chModelName));
        SN = std::string((LPCSTR)(pDeviceInfo->SpecialInfo.stCXPInfo.chSerialNumber));
    }
    else if (pDeviceInfo->nTLayerType == MV_GENTL_XOF_DEVICE)
    {
        UDN = std::string((LPCSTR)(pDeviceInfo->SpecialInfo.stXoFInfo.chUserDefinedName));
        MN = std::string((LPCSTR)(pDeviceInfo->SpecialInfo.stXoFInfo.chModelName));
        SN = std::string((LPCSTR)(pDeviceInfo->SpecialInfo.stXoFInfo.chSerialNumber));
    }

    if (strcmp("", UDN.c_str()) != 0)
    {
        res = UDN + "(" + SN + ")";
    }
    else
    {
        res = MN + "(" + SN + ")";
    }
    return res;
}

std::map<std::string, std::string> VIS::VIS_HICON_READER::FindHiconCamera() {
    MV_CC_Initialize();
    MV_CC_DEVICE_INFO_LIST  m_stDevList;
    std::map<std::string, std::string> device_list;
    memset(&m_stDevList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE | MV_GENTL_GIGE_DEVICE | MV_GENTL_CAMERALINK_DEVICE |
        MV_GENTL_CXP_DEVICE | MV_GENTL_XOF_DEVICE, &m_stDevList);
    if (MV_OK != nRet) return device_list;
    for (unsigned int i = 0; i < m_stDevList.nDeviceNum; i++)
    {
        std::string strMsg;
        MV_CC_DEVICE_INFO* pDeviceInfo = m_stDevList.pDeviceInfo[i];
        if (!pDeviceInfo) continue;
        std::string pUserName = GetDeviceName(pDeviceInfo);
        if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
        {
            int nIp1 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
            int nIp2 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
            int nIp3 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
            int nIp4 = (pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);
            std::ostringstream oss;
            oss << "Industrial Camera" << '[' << i << "] GigE:    " << pUserName << "  ("
                << nIp1 << '.' << nIp2 << '.' << nIp3 << '.' << nIp4 << ')';
            strMsg = oss.str();
        }
        else if (pDeviceInfo->nTLayerType == MV_USB_DEVICE)
        {
            std::ostringstream oss;
            oss <<"Industrial Camera" << '[' << i << "] UsbV3:    " << pUserName;
            strMsg = oss.str();
        }
        else if (pDeviceInfo->nTLayerType == MV_GENTL_CAMERALINK_DEVICE)
        {
            std::ostringstream oss;
            oss << "Industrial Camera" << '[' << i << "] CML:    " << pUserName;
            strMsg = oss.str();
        }
        else if (pDeviceInfo->nTLayerType == MV_GENTL_CXP_DEVICE)
        {
            std::ostringstream oss;
            oss << "Industrial Camera" << '[' << i << "] CXP:    " << pUserName;
            strMsg = oss.str();
        }
        else if (pDeviceInfo->nTLayerType == MV_GENTL_XOF_DEVICE)
        {
            std::ostringstream oss;
            oss << "Industrial Camera" << '[' << i << "] Xof:    " << pUserName;
            strMsg = oss.str();
        }
        else
        {
            std::cerr << "Unknown device enumerated" << std::endl;
            continue;
        }
        device_list[strMsg] = pUserName;
    }

    return device_list;
}

void VIS::VIS_HICON_READER::ShowErrorMsg(int nErrorNum)
{
    std::string errorMsg = "";
    switch (nErrorNum)
    {
    case MV_E_HANDLE:           errorMsg += "Error or invalid handle ";                                         break;
    case MV_E_SUPPORT:          errorMsg += "Not supported function ";                                          break;
    case MV_E_BUFOVER:          errorMsg += "Cache is full ";                                                   break;
    case MV_E_CALLORDER:        errorMsg += "Function calling order error ";                                    break;
    case MV_E_PARAMETER:        errorMsg += "Incorrect parameter ";                                             break;
    case MV_E_RESOURCE:         errorMsg += "Applying resource failed ";                                        break;
    case MV_E_NODATA:           errorMsg += "No data ";                                                         break;
    case MV_E_PRECONDITION:     errorMsg += "Precondition error, or running environment changed ";              break;
    case MV_E_VERSION:          errorMsg += "Version mismatches ";                                              break;
    case MV_E_NOENOUGH_BUF:     errorMsg += "Insufficient memory ";                                             break;
    case MV_E_ABNORMAL_IMAGE:   errorMsg += "Abnormal image, maybe incomplete image because of lost packet ";   break;
    case MV_E_UNKNOW:           errorMsg += "Unknown error ";                                                   break;
    case MV_E_GC_GENERIC:       errorMsg += "General error ";                                                   break;
    case MV_E_GC_ACCESS:        errorMsg += "Node accessing condition error ";                                  break;
    case MV_E_ACCESS_DENIED:	errorMsg += "No permission ";                                                   break;
    case MV_E_BUSY:             errorMsg += "Device is busy, or network disconnected ";                         break;
    case MV_E_NETER:            errorMsg += "Network error ";                                                   break;
    }

    std::cerr << errorMsg << std::endl;
}

void VIS::VIS_HICON_READER::Stop(){
    CloseCam();
}

void VIS::VIS_HICON_READER::CloseCam() {
    MV_CC_StopGrabbing(handle);
    MV_CC_CloseDevice(handle);
    MV_CC_DestroyHandle(handle);
}

bool VIS::VIS_HICON_READER::Init() {
    MV_CC_DEVICE_INFO_LIST  m_stDevList;
    memset(&m_stDevList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE | MV_GENTL_GIGE_DEVICE | MV_GENTL_CAMERALINK_DEVICE |
        MV_GENTL_CXP_DEVICE | MV_GENTL_XOF_DEVICE, &m_stDevList);
    if (MV_OK != nRet) return false;
    for (unsigned int i = 0; i < m_stDevList.nDeviceNum; i++)
    {
        std::string strMsg;
        MV_CC_DEVICE_INFO* _pDeviceInfo = m_stDevList.pDeviceInfo[i];
        if (!_pDeviceInfo) continue;
        std::string pUserName = GetDeviceName(_pDeviceInfo);
        if (devicename == pUserName) {
            pDeviceInfo = _pDeviceInfo;
            break;
        };
    }
    if (!pDeviceInfo) return false;
    nRet = MV_CC_CreateHandle(&handle, pDeviceInfo);
    if (nRet != MV_OK || !handle) {
        ShowErrorMsg(nRet);
        return 0;
    }
    if (MV_CC_OpenDevice(handle) != MV_OK) return false;

    if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE) {
        int packetSize = MV_CC_GetOptimalPacketSize(handle);
        if (packetSize > 0)
            MV_CC_SetIntValueEx(handle, "GevSCPSPacketSize", packetSize);
    }
    MV_CC_SetEnumValue(handle, "TriggerMode", 0);
    MV_CC_SetBoolValue(handle, "AcquisitionFrameRateEnable", true);
    MV_CC_SetFloatValue(handle, "AcquisitionFrameRate", _framerate);

    nRet = MV_CC_StartGrabbing(handle);
    if (nRet != MV_OK) {
        ShowErrorMsg(nRet);
        return true;
    }
    return true;
};

bool VIS::VIS_HICON_READER::Capture(uint64_t& ImgT, cv::Mat& frame) {
    int ret = MV_CC_GetImageBuffer(handle, &stOutFrame, 10000);
    if (ret == MV_OK) {
        pData = (unsigned char*)stOutFrame.pBufAddr;
        pInfo = &stOutFrame.stFrameInfo;
        if (!pData || !pInfo) {
            return false;
        }

        ImgT = (uint64_t(pInfo->nDevTimeStampHigh) << 32) |
            (uint64_t(pInfo->nDevTimeStampLow));
        ImgT *= 10;

        if (isFirst) {
            BaseTime = duration_cast<nanoseconds>(
                high_resolution_clock::now().time_since_epoch()
            ).count() - BASE::ProjectStartTime;
            FirstFrameTime = ImgT;
            isFirst = false;
        }
        ImgT = ImgT - FirstFrameTime + BaseTime;
        toCvMat(frame);
        MV_CC_FreeImageBuffer(handle, &stOutFrame);
        return true;
    }
    else {
        ShowErrorMsg(ret);
        return false;
    }
}

void VIS::VIS_HICON_READER::toCvMat(cv::Mat & frame) {
    std::vector<uint8_t> bgrBuf(pInfo->nWidth * pInfo->nHeight * 3);
    MV_CC_PIXEL_CONVERT_PARAM stParam = { 0 };
    stParam.nWidth = pInfo->nWidth;
    stParam.nHeight = pInfo->nHeight;
    stParam.pSrcData = pData;
    stParam.nSrcDataLen = pInfo->nFrameLen;
    stParam.enSrcPixelType = pInfo->enPixelType;
    stParam.enDstPixelType = PixelType_Gvsp_BGR8_Packed;
    stParam.pDstBuffer = bgrBuf.data();
    stParam.nDstBufferSize = bgrBuf.size();
    MV_CC_ConvertPixelType(handle, &stParam);

    cv::Mat tmp(
        pInfo->nHeight,
        pInfo->nWidth,
        CV_8UC3,
        bgrBuf.data()
    );

    if (outformat == "GRAY8") cv::cvtColor(tmp, frame, cv::COLOR_BGR2GRAY);
    else if (outformat == "RGB") cv::cvtColor(tmp, frame, cv::COLOR_BGR2RGB);
    else frame = tmp;
    cv::resize(frame, frame, cv::Size(outwidth, outheight));
}