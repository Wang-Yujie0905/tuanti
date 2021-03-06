#include "boardwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFile>
#include <QDataStream>
#include <QMediaPlayer>
#include <QDebug>
#include <QMediaPlaylist>

/*类静态数据成员定义*/
const QSize BoardWidget::WIDGET_SIZE(430, 430);
const QSize BoardWidget::CELL_SIZE(25, 25);
const QPoint BoardWidget::START_POS(40, 40);
const QPoint BoardWidget::ROW_NUM_START(15, 45);
const QPoint BoardWidget::CLU_NUM_START(39, 25);
const int BoardWidget::BOARD_WIDTH;
const int BoardWidget::BOARD_HEIGHT;
const int BoardWidget::NO_PIECE;
const int BoardWidget::WHITE_PIECE;
const int BoardWidget::BLACK_PIECE;
const bool BoardWidget::WHITE_PLAYER;
const bool BoardWidget::BLACK_PLAYER;

BoardWidget::BoardWidget(QWidget *parent) :
    QWidget(parent),
    trackPos(28, 28)
{
    setFixedSize(WIDGET_SIZE);
    setMouseTracking(true);

    initBoard();
}

void BoardWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(0, 0, width(), height(), Qt::lightGray);	//背景颜色

        /*QPainter painter(this);
        painter.drawPixmap(0,0,this->width(),this->height(),QPixmap(":/images/frame.jpg"));*/


    for (int i = 0; i < BOARD_WIDTH; i++)
    {
        painter.drawText(CLU_NUM_START + QPoint(i * CELL_SIZE.width(), 0),
                         QString::number(i + 1));
    }
    for (int i = 0; i < BOARD_HEIGHT; i++)
    {
        painter.drawText(ROW_NUM_START + QPoint(0, i * CELL_SIZE.height()),
                         QString::number(i + 1));
    }

    for (int i = 0; i < BOARD_WIDTH - 1; i++)	//绘制棋盘格子
    {
        for (int j = 0; j < BOARD_HEIGHT - 1; j++)
        {
            painter.drawRect(QRect(START_POS + QPoint(i * CELL_SIZE.width(), j * CELL_SIZE.height()),
                                   CELL_SIZE));
        }
    }

    painter.setPen(Qt::red);
    QPoint poses[12] = {
        trackPos + QPoint(0, 8),
        trackPos,
        trackPos + QPoint(8, 0),
        trackPos + QPoint(17, 0),
        trackPos + QPoint(25, 0),
        trackPos + QPoint(25, 8),
        trackPos + QPoint(25, 17),
        trackPos + QPoint(25, 25),
        trackPos + QPoint(17, 25),
        trackPos + QPoint(8, 25),
        trackPos + QPoint(0, 25),
        trackPos + QPoint(0, 17)
    };
    painter.drawPolyline(poses, 3);
    painter.drawPolyline(poses + 3, 3);
    painter.drawPolyline(poses + 6, 3);
    painter.drawPolyline(poses + 9, 3);

    painter.setPen(Qt::NoPen);
    for (int i = 0; i < BOARD_WIDTH; i++)	//绘制棋子
    {
        for (int j = 0; j < BOARD_HEIGHT; j++)
        {
            if (board[i][j] != NO_PIECE)
            {
                QColor color = (board[i][j] == WHITE_PIECE) ? Qt::white : Qt::black;
                painter.setBrush(QBrush(color));
                painter.drawEllipse(START_POS.x() - CELL_SIZE.width()/2 + i*CELL_SIZE.width(),
                                    START_POS.y() - CELL_SIZE.height()/2 + j*CELL_SIZE.height(),
                                    CELL_SIZE.width(), CELL_SIZE.height());
            }
        }
    }

    painter.setPen(Qt::red);
    if (!dropedPieces.isEmpty())
    {
        QPoint lastPos = dropedPieces.top();
        QPoint drawPos = START_POS + QPoint(lastPos.x() * CELL_SIZE.width(), lastPos.y() * CELL_SIZE.height());
        painter.drawLine(drawPos + QPoint(0, 5), drawPos + QPoint(0, -5));
        painter.drawLine(drawPos + QPoint(5, 0), drawPos + QPoint(-5, 0));
    }

    for (QPoint pos : winPoses)
    {
        QPoint drawPos = START_POS + QPoint(pos.x() * CELL_SIZE.width(), pos.y() * CELL_SIZE.height());
        painter.drawLine(drawPos + QPoint(0, 5), drawPos + QPoint(0, -5));
        painter.drawLine(drawPos + QPoint(5, 0), drawPos + QPoint(-5, 0));
    }
}

void BoardWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (receivePlayers.contains(nextPlayer) && !endGame)
    {
        QPoint pos = event->pos() - START_POS;
        int x = pos.x();
        int y = pos.y();
        int pieceX = x / CELL_SIZE.width();
        int pieceY = y / CELL_SIZE.height();
        int offsetX = x % CELL_SIZE.width();
        int offsetY = y % CELL_SIZE.height();
        if (offsetX > CELL_SIZE.width() / 2)
        {
            pieceX++;
        }
        if (offsetY > CELL_SIZE.height() / 2)
        {
            pieceY++;
        }
        downPiece(pieceX, pieceY);
    }
}

void BoardWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos() - START_POS + QPoint(CELL_SIZE.width()/2, CELL_SIZE.height()/2);
    int x = pos.x();
    int y = pos.y();
    //超过范围
    if (x < 0 || x >= CELL_SIZE.width() * BOARD_WIDTH ||
            y < 0 || y >= CELL_SIZE.height() * BOARD_HEIGHT)
    {
        return;
    }
    int offsetX = x % CELL_SIZE.width();
    int offsetY = y % CELL_SIZE.height();
    setTrackPos(QPoint(x - offsetX, y - offsetY) + START_POS - QPoint(CELL_SIZE.width()/2, CELL_SIZE.height()/2));
}


void BoardWidget::initBoard()
{
    receivePlayers << WHITE_PLAYER << BLACK_PLAYER;
    newGame();
}

void BoardWidget::downPiece(int x, int y)
{
    if (x >= 0 && x < BOARD_WIDTH && y >= 0 && y < BOARD_HEIGHT && board[x][y] == NO_PIECE)
    {
        dropedPieces.push(QPoint(x, y));
        board[x][y] = (nextPlayer == WHITE_PLAYER) ? WHITE_PIECE : BLACK_PIECE;
        update();
        checkWinner();
        if (!endGame)
        {
            switchNextPlayer();
        }
    }
}

void BoardWidget::undo(int steps)
{
    if (!endGame)
    {
        for (int i = 0; i < steps && !dropedPieces.isEmpty(); i++)
        {
            QPoint pos = dropedPieces.pop();
            board[pos.x()][pos.y()] = NO_PIECE;
        }

        update();
        switchNextPlayer();
    }
}

void BoardWidget::setTrackPos(const QPoint &value)
{
    trackPos = value;
    update();
}

void BoardWidget::setReceivePlayers(const QSet<int> &value)
{
    receivePlayers = value;
}

Board BoardWidget::getBoard()
{
    return board;
}

void BoardWidget::switchNextPlayer()
{
    nextPlayer = !nextPlayer;
    emit turnNextPlayer(nextPlayer);
}

void BoardWidget::checkWinner()
{
    bool fullPieces = true;
    for (int i = 0; i < BOARD_WIDTH; i++)
    {
        for (int j = 0; j < BOARD_HEIGHT; j++)
        {
            if (board[i][j] == NO_PIECE)
            {
                fullPieces = false;
            }
            if (board[i][j] != NO_PIECE && isFivePieceFrom(i, j))
            {
                bool winner = (board[i][j] == WHITE_PIECE) ? WHITE_PLAYER : BLACK_PLAYER;
                endGame = true;
                emit gameOver(winner);
            }
        }
    }
    if (fullPieces)
    {
        endGame = true;
        emit gameOver(2);   //代表和棋
    }
}

bool BoardWidget::isFivePieceFrom(int x, int y)
{
    return isVFivePieceFrom(x, y) || isHFivePieceFrom(x, y) || isFSFivePieceFrom(x, y) || isBSFivePieceFrom(x, y);
}

bool BoardWidget::isVFivePieceFrom(int x, int y)
{
    int piece = board[x][y];
    for (int i = 1; i < 5; i++)
    {
        if (y + i >= BOARD_HEIGHT || board[x][y + i] != piece)
        {
            return false;
        }
    }
    winPoses.clear();
    for (int i = 0; i < 5; i++)
    {
        winPoses.append(QPoint(x, y + i));
    }
    return true;
}

bool BoardWidget::isHFivePieceFrom(int x, int y)
{
    int piece = board[x][y];
    for (int i = 1; i < 5; i++)
    {
        if (x + i >= BOARD_WIDTH || board[x + i][y] != piece)
        {
            return false;
        }
    }
    winPoses.clear();
    for (int i = 0; i < 5; i++)
    {
        winPoses.append(QPoint(x + i, y));
    }
    return true;
}

bool BoardWidget::isFSFivePieceFrom(int x, int y)
{
    int piece = board[x][y];
    for (int i = 1; i < 5; i++)
    {
        if (x + i >= BOARD_WIDTH || y - i < 0 || board[x + i][y - i] != piece)
        {
            return false;
        }
    }
    winPoses.clear();
    for (int i = 0; i < 5; i++)
    {
        winPoses.append(QPoint(x + i, y - i));
    }
    return true;
}

bool BoardWidget::isBSFivePieceFrom(int x, int y)
{
    int piece = board[x][y];
    for (int i = 1; i < 5; i++)
    {
        if (x + i >= BOARD_WIDTH || y + i >= BOARD_HEIGHT || board[x + i][y + i] != piece)
        {
            return false;
        }
    }
    winPoses.clear();
    for (int i = 0; i < 5; i++)
    {
        winPoses.append(QPoint(x + i, y + i));
    }
    return true;
}

void BoardWidget::newGame()
{
    for (int i = 0; i < BOARD_WIDTH; i++)
    {
        for (int j = 0; j < BOARD_HEIGHT; j++)
        {
            board[i][j] = NO_PIECE;
        }
    }
    winPoses.clear();
    dropedPieces.clear();
    nextPlayer = BLACK_PLAYER;
    endGame = false;
    update();
    emit turnNextPlayer(nextPlayer);

    QMediaPlaylist *playList = new QMediaPlaylist(this);
    playList->addMedia(QUrl("qrc:///music/music.wav"));
    playList->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop); //单曲循环
    QMediaPlayer *player = new QMediaPlayer(this);
    player->setPlaylist(playList);
    player->setVolume(30);

    player->play();

}
/*bool BoardWidget::nohands(int row,int col){
    int count = 0;                                     //禁手最少四子相连，循环四次
            int num(0);
            int numm(0);//记录几个方向出现四子及以上相连
            int winflag = 1;                                   //记录连续棋子相连个数
            int i, j;
            int cur;
            cur=1;
            for (i = row-1, j = col; i > 0 && count++ < 5; i--)   //竖向判断
            {
                if (board[i][j] == cur)
                    winflag++;
                else
                    break;
            }
            count = 0;
            for (i = row+1, j = col; i < 21 && count++ < 5; i++)
            {
                if (board[i][j] == cur)
                    winflag++;
                else
                    break;
            }
            count = 0;
            if (winflag == 4 ) {
                num++;                                    //出现则次数加一
                winflag = 1;
            }
            else if(winflag==5){
                numm++;
                winflag=1;
            }

            else
                winflag = 1;
            for (i = row, j = col+1; j < 21 && count++ < 5; j++)    //横向判断
            {
                if (board[i][j] == cur)
                    winflag++;
                else
                    break;
            }
            count = 0;
            for (i = row, j = col-1; j > 0 && count++ < 5; j--)
            {
                if (board[i][j] == cur)
                    winflag++;
                else
                    break;
            }
            count = 0;
            if (winflag ==4) {
                num++;
                winflag = 1;
            }
            else if(winflag==5){
                numm++;
                winflag=1;
            }
            else
                winflag = 1;
            for (i = row+1, j = col+1; i < 21 && j < 21 && count++ < 5; i++, j++)
            {
                if (board[i][j] == cur)
                    winflag++;
                else
                    break;
            }
            count = 0;
            for (i = row-1, j = col-1; i > 0 && j > 0 && count++ < 5; i--, j--)  //左斜判断
            {
                if (board[i][j] == cur)
                    winflag++;
                else
                    break;
            }
            count = 0;
            if (winflag ==4) {
                num++;
                winflag = 1;
            }
            else if(winflag==5) {
                numm++;
                winflag=1;
            }
            else
                winflag = 1;
            for (i = row-1, j = col+1; i > 0 && j < 21 && count++ < 5; i--, j++)   //右斜判断
            {
                if (board[i][j] == cur)
                    winflag++;
                else
                    break;
            }
            count = 0;
            for (i = row+1, j = col-1; i < 21 && j > 0 && count++ < 5; i++, j--)
            {
                if (board[i][j] == cur)
                    winflag++;
                else
                    break;
            }
            count = 0;
            if (winflag ==4) {
                num++;
                winflag = 1;
            }
            else if(winflag==5){
                numm++;
                winflag=1;
            }
            else
                winflag = 1;
            if (num >= 2||numm>=2) return true;      //出现最少两个方向禁手，则返回白子胜利
            else return false;
    }*/

