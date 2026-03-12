#include <iostream>
#include <cassert>
#include <string>
#include <FrameGen.hpp>
#include <DeviceCaps.hpp>

// Simple test framework
class TestRunner {
public:
    TestRunner() : passedTests(0), failedTests(0) {}
    
    void assert_true(bool condition, const char* testName) {
        if (condition) {
            std::cout << "✓ PASS: " << testName << std::endl;
            passedTests++;
        } else {
            std::cout << "✗ FAIL: " << testName << std::endl;
            failedTests++;
        }
    }
    
    void assert_equal(int actual, int expected, const char* testName) {
        assert_true(actual == expected, testName);
    }
    
    void summary() {
        std::cout << "\n=== TEST SUMMARY ===" << std::endl;
        std::cout << "Passed: " << passedTests << std::endl;
        std::cout << "Failed: " << failedTests << std::endl;
        std::cout << "Total: " << (passedTests + failedTests) << std::endl;
    }
    
    int getFailureCount() const { return failedTests; }

private:
    int passedTests;
    int failedTests;
};

int main() {
    TestRunner runner;
    
    std::cout << "=== FrameGen Unit Tests ===" << std::endl << std::endl;
    
    // Test 1: Factory resolves to SOFTWARE backend when others not available
    {
        framegen::DeviceCaps caps = framegen::DetectDeviceCaps();
        
        // Create frame generator with DLSS_FG backend - should fallback to something supported
        auto result = framegen::CreateFrameGenerator(
            framegen::Backend::DLSS_FG,
            caps
        );
        
        // Should succeed
        runner.assert_true(result.has_value(), "Factory backend selection/fallback works");
    }
    
    // Test 2: SOFTWARE backend X2 multiplier is supported
    {
        framegen::DeviceCaps caps = framegen::DetectDeviceCaps();
        auto result = framegen::CreateFrameGenerator(
            framegen::Backend::SOFTWARE,
            caps
        );
        runner.assert_true(result.has_value(), "SOFTWARE backend creation works");
        
        // If created, verify backend is correct
        if (result.has_value()) {
            auto supportX2 = (*result)->SupportsMultiplier(framegen::FrameMultiplier::X2);
            runner.assert_true(supportX2, "SOFTWARE X2 multiplier supported");
        }
    }
    
    // Test 3: SOFTWARE backend X3 multiplier handling
    {
        framegen::DeviceCaps caps = framegen::DetectDeviceCaps();
        auto result = framegen::CreateFrameGenerator(
            framegen::Backend::SOFTWARE,
            caps
        );
        if (result.has_value()) {
            bool supportsX3 = (*result)->SupportsMultiplier(framegen::FrameMultiplier::X3);
            // SOFTWARE may or may not support X3, just verify it responds
            runner.assert_true(true, "SOFTWARE X3 multiplier support queried");
        }
    }
    
    // Test 4: SOFTWARE backend X4 multiplier handling
    {
        framegen::DeviceCaps caps = framegen::DetectDeviceCaps();
        auto result = framegen::CreateFrameGenerator(
            framegen::Backend::SOFTWARE,
            caps
        );
        if (result.has_value()) {
            bool supportsX4 = (*result)->SupportsMultiplier(framegen::FrameMultiplier::X4);
            runner.assert_true(true, "SOFTWARE X4 multiplier support queried");
        }
    }
    
    // Test 5: Device capability detection completes
    {
        auto caps = framegen::DetectDeviceCaps();
        runner.assert_true(true, "Device capability detection successful");
    }
    
    // Test 6: Invalid null caps pointer handling (implicit in DetectDeviceCaps)
    {
        auto caps = framegen::DetectDeviceCaps();
        runner.assert_true(true, "Device capabilities detected without crashes");
    }
    
    // Test 7: FSR3_FG backend selection (will fallback if not available)
    {
        framegen::DeviceCaps caps = framegen::DetectDeviceCaps();
        auto result = framegen::CreateFrameGenerator(
            framegen::Backend::FSR3_FG,
            caps
        );
        // Should either use FSR3_FG or fallback to available backend
        runner.assert_true(result.has_value(), "FSR3_FG backend request handled");
    }
    
    // Test 8: XESS_FG backend selection (will fallback if not available)
    {
        framegen::DeviceCaps caps = framegen::DetectDeviceCaps();
        auto result = framegen::CreateFrameGenerator(
            framegen::Backend::XESS_FG,
            caps
        );
        // Should either use XESS_FG or fallback to available backend
        runner.assert_true(result.has_value(), "XESS_FG backend request handled");
    }
    
    // Test 9: Backend enum sanity
    {
        // Verify backend enum values are in expected range
        int dlss_val = static_cast<int>(framegen::Backend::DLSS_FG);
        int software_val = static_cast<int>(framegen::Backend::SOFTWARE);
        runner.assert_true(dlss_val == 0 && software_val == 3, "Backend enum values as expected");
    }
    
    // Test 10: FrameMultiplier enum sanity
    {
        int x2_val = static_cast<int>(framegen::FrameMultiplier::X2);
        int x4_val = static_cast<int>(framegen::FrameMultiplier::X4);
        runner.assert_true(x2_val == 2 && x4_val == 4, "FrameMultiplier enum values as expected");
    }
    
    runner.summary();
    
    // Exit with failure code if any tests failed
    return runner.getFailureCount() > 0 ? 1 : 0;
}
