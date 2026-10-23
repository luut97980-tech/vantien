# VTMenu iOS — bản sửa R5.3

## Triệu chứng

Gói game thực tế trên máy là `Payload/PLAYTOGETHERVNG.app` với bundle id
`ptg.vng` (bản VNG/repack), không phải `com.vng.playtogether`. Vì vậy:

- `tao_ipa_menu.py` dừng ở bước kiểm tra Info.plist: “Chi nhan PLAY TOGETHER VNG
  2.32.0 (227628)”.
- Nếu tiêm tay, guard trong game cũng từ chối metadata và backend không kết nối.

## Nguyên nhân

Cùng build 2.32.0 (227628) được phát hành dưới hai bundle id. Bản `ptg.vng` có
đúng framework UUID `1c1d8aea78c838f89a09d62cc639465d`, đúng
`global-metadata.dat` SHA-256 `ddefd97ac49d5431d25757e409e4c03973c79f553c025b931da00949867cc5fb`
và đúng toàn bộ RVA method trong `analysis/ios_build.json`; chỉ khác chuỗi bundle
id. Tool và guard lại so sánh cứng một chuỗi.

## Đã sửa

- `src/app_metadata.hpp`: `ExpectedBundles` gồm `com.vng.playtogether` và
  `ptg.vng`; `bundleAccepted()` dùng chung cho `matchesExpected()`.
- `src/app_metadata_apple.hpp`: dòng “Expected” in cả hai bundle.
- `src/Plugin.mm`: nhãn chẩn đoán `PTG iOS R5.3`.
- `src/mobile_menu.cpp`, `src/mobile_menu.hpp`: nhãn `PTG | R5.3`.
- `tao_ipa_menu.py`: `EXPECTED_BUNDLES` gồm hai giá trị; kiểm tra manifest và
  Info.plist bằng `in`.
- `tests/app_metadata_test.cpp`: thêm ca `ptg.vng` hợp lệ; bundle lạ vẫn bị từ chối.
- `README.md`, `HUONG_DAN_WINDOWS.md`: ghi rõ giữ nguyên bundle gốc của IPA.

Version 2.32.0, build 227628, framework UUID, metadata hash, chữ ký hàm và
`menu_revision=5` vẫn kiểm tra như cũ.

## Build lại

Giữ nguyên quy trình: GitHub Actions → artifact **VTMenu-iOS-ESP-Spam-R5** →
`tao_ipa_menu.py` → ký ESign. Tool trong artifact đã là bản R5.3.

## Kiểm tra sau khi cài

1. Mở game, chưa bật gì, bấm đăng nhập → vào được, không văng.
2. Mở menu: tiêu đề `PTG | R5.3`, trạng thái “Đã kết nối”.
3. Bật ESP → khung hiện sau ~0,75 s; bật Spam nhảy → nhảy theo nhịp.
4. Xoay máy: bảng menu đứng đúng hướng, nằm trong vùng an toàn.
5. Nếu chẩn đoán vẫn báo sai metadata, gửi nguyên nội dung Sao chép chẩn đoán:
   dòng “Installed plist” và “Expected” cho biết bundle nào đang bị lệch.


## Sửa lỗi crash văng khởi động (UIViewControllerHierarchyInconsistency trên iOS 18)

- **Triệu chứng:** Mở game khoảng 3,5 giây là bị văng đột ngột (SIGABRT / Abort trap: 6). Log .ips ghi nhận ngoại lệ `UIViewControllerHierarchyInconsistency` tại `VTAttach()` khi gọi `addSubview:`.
- **Nguyên nhân:** Bản R5.2 khai báo `[parent addChildViewController:VTController]` nhưng lại gắn view của nó vào `UIWindow` (`container = game`), vi phạm ràng buộc phân cấp giao diện của UIKit.
- **Giải pháp:** Chuyển sang cơ chế `VTPassthroughWindow` (UIWindow overlay riêng gắn trực tiếp vào `UIWindowScene` của game). `VTController` trở thành `rootViewController` của overlay window, độc lập hoàn toàn với `parent` của Unity:
  - Loại bỏ triệt để lỗi xung đột phân cấp `UIViewControllerHierarchyInconsistency`.
  - Không bị ảnh hưởng bởi transform xoay của `UnityView`.
  - Cảm ứng ngoài vùng menu được chuyển tiếp tự nhiên xuống window game bên dưới.
