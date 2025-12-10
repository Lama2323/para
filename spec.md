**Mô phỏng nhiều trận game song song với rollback và task-based parallelism dùng scheduler work-stealing**


**Mục tiêu**
- Tập dùng thread, mutex, atomic, thread pool.
- Thấy rõ sự khác biệt concurrency (tránh race condition) bằng parallelism (tận dụng nhiều core).
- dùng cơ chế task-based parallelism dùng scheduler work-stealing để phân phối task

**Đề bài**

Demo server xử lý giả lập 20 match, 40 client, mỗi client gửi 10000 input 

Input gồm lên, xuống, trái, phải, tấn công 1, tấn công 2

Tấn công 1 đánh 10 sát thương, ko hồi chiêu
Tấn công 2 đánh 30 sát thương, 1s hồi

Có 4 task cần mô phỏng:
Trong server game:
- Nhận input
- Ghép input vào đúng match
- Xử lý trạng thái match
Mô phỏng client
- Gửi input

Đo thời gian xử lý hết tổng 400000 input giữa tuần tự và parallel

**Rollback**
- Mỗi trận cứ 5 tick thì rollback 1 lần.
- Lưu snapshot state vào vector.
- Khi rollback: lock state, load snapshot, re-simulate.

1. **Action của Hệ thống (System-level Actions)**

Mấy cái này không phải gameplay mà để quản lý nhiều trận song song:

- **CREATE_MATCH** -> tạo 1 trận mới (gán ID, tick counter riêng).
- **START_MATCH / END_MATCH** -> khởi động và kết thúc game.
- **SYNC_TICK** -> đồng bộ tick giữa server và client (quan trọng cho rollback).


2. **Action trong Gameplay (Game-level Actions)**

**Movement**
- **MOVE_LEFT, MOVE_RIGHT, MOVE_UP, MOVE_DOWN**

**Môi trường**
- đấu trường có diện tích 20x20, player không di chuyển khỏi diện tích này
- player không có máu để bị trừ

**Combat**
- **ATTACK_LIGHT** (đánh thường).
- **ATTACK_HEAVY** (gây delay 1s -> input rollback dễ thấy).

3. **Payload gợi ý mỗi action**

struct Input {
    int matchId;   // để biết action thuộc trận nào
    int playerId;  
    int tickId;       // tick mà action sinh ra (rollback dựa vào đây)
    ActionType type; // MOVE, ATTACK,...
    int dx, dy;     // dùng cho MOVE
};

4. **Rollback logic khi mô phỏng nhiều trận**

- Khi input tới trễ -> server rollback state của match đó về tickId, rồi simulate lại -> không ảnh hưởng đến match khác.