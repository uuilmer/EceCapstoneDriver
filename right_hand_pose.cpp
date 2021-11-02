#include "openvr_driver.h"
#include <thread>
#include <atomic>
#include <chrono>

using namespace vr;
using namespace std;

#define DEVICE_NAME "right_hand_pose"

static const char *device_serial_number = DEVICE_NAME "SN0";

class RightHandPose : public ITrackedDeviceServerDriver
{
public:
    uint32_t driverId;
    DriverPose_t m_pose;
    thread m_pose_thread;
    bool m_active;
    int m_frame_count;

    RightHandPose()
        : m_frame_count(0),
          m_active(false)
    {
    }

    EVRInitError Activate(uint32_t unObjectId)
    {
        driverId = unObjectId; //unique ID for your driver

        m_pose.poseIsValid = true;
        m_pose.result = TrackingResult_Running_OK;
        m_pose.deviceIsConnected = true;
        m_pose.qWorldFromDriverRotation.w = 1;
        m_pose.qDriverFromHeadRotation.w = 1;
        m_pose.vecPosition[0] = 0;
        m_pose.vecPosition[1] = -.5;
        m_pose.vecPosition[2] = -1.5;

        PropertyContainerHandle_t props = VRProperties()->TrackedDeviceToPropertyContainer(driverId);

        VRProperties()->SetStringProperty(props, Prop_InputProfilePath_String, "{example}/input/controller_profile.json");
        VRProperties()->SetInt32Property(props, Prop_ControllerRoleHint_Int32, TrackedControllerRole_LeftHand);

        m_active = true;
        m_pose_thread = std::thread([]()
                                    {
                                        while (true)
                                        {
                                            //     m_frame_count++;
                                            //     m_pose.vecPosition[0] = 30;
                                            //     m_pose.vecPosition[1] = 30;
                                            //     m_pose.vecPosition[2] = 30;
                                            //     VRServerDriverHost()->TrackedDevicePoseUpdated(driverId, m_pose, sizeof(DriverPose_t));
                                            //     this_thread::sleep_for(chrono::milliseconds(11));
                                        }
                                    });

        return VRInitError_None;
    }

    void Deactivate()
    {
        if (m_active)
        {
            m_active = false;
            m_pose_thread.join();
        }
    }

    void EnterStandby() {}
    void *GetComponent(const char *pchComponentNameAndVersion)
    {
        return nullptr;
    }

    void DebugRequest(const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize) {}
    DriverPose_t GetPose() { return m_pose; }
};

////////////////////////////////////////////////////////////////////////////////////
// DeviceProvider : Notifies vrserver of the presence of devices
//                  For this Pose, a single device is assumed to be present and so
//                  vrserver is notified of it.
////////////////////////////////////////////////////////////////////////////////////
class DeviceProvider : public IServerTrackedDeviceProvider
{
public:
    RightHandPose m_device;
    EVRInitError Init(IVRDriverContext *pDriverContext)
    {
        VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
        bool rc = VRServerDriverHost()->TrackedDeviceAdded(device_serial_number, TrackedDeviceClass_Controller, &m_device);
        return VRInitError_None;
    }
    void Cleanup() {}
    const char *const *GetInterfaceVersions()
    {
        return k_InterfaceVersions;
    }
    void RunFrame() {}
    bool ShouldBlockStandbyMode() { return false; }
    void EnterStandby() {}
    void LeaveStandby() {}
} g_device_provider;

#if defined(_WIN32)
#define HMD_DLL_EXPORT extern "C" __declspec(dllexport)
#define HMD_DLL_IMPORT extern "C" __declspec(dllimport)
#elif defined(__GNUC__) || defined(COMPILER_GCC) || defined(__APPLE__)
#define HMD_DLL_EXPORT extern "C" __attribute__((visibility("default")))
#define HMD_DLL_IMPORT extern "C"
#else
#error "Unsupported Platform."
#endif

////////////////////////////////////////////////////////////////////////////////////
// HmdDriverFactory : DLL entrypoint called by vrserver
////////////////////////////////////////////////////////////////////////////////////
HMD_DLL_EXPORT void *HmdDriverFactory(const char *pInterfaceName, int *pReturnCode)
{
    if (0 == strcmp(IServerTrackedDeviceProvider_Version, pInterfaceName))
    {
        return &g_device_provider;
    }
    return nullptr;
}
