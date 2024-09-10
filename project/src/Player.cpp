/*
Copyright (c) 2000-2003, Jelle Kok, University of Amsterdam
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the University of Amsterdam nor the names of its
contributors may be used to endorse or promote products derived from this
software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*! \file Player.cpp

<pre>
<b>File:</b>          Player.cpp
<b>Project:</b>       Robocup Soccer Simulation Team: UvA Trilearn
<b>Authors:</b>       Jelle Kok
<b>Created:</b>       03/03/2001
<b>Last Revision:</b> $ID$
<b>Contents:</b>      This file contains the definitions for the Player class,
               which is a superclass from BasicPlayer and contains the
               decision procedure to select the skills from the
               BasicPlayer.
<hr size=2>
<h2><b>Changes</b></h2>
<b>Date</b>             <b>Author</b>          <b>Comment</b>
03/03/2001       Jelle Kok       Initial version created
</pre>
*/
#include "Player.h"
#include "Parse.h"
#ifndef WIN32
  #include <sys/poll.h> // needed for 'poll'
#endif

#include <string.h>
#include <stdlib.h>

extern Logger LogDraw;

/*!This is the constructor the Player class and calls the constructor of the
   superclass BasicPlayer.
   \param act ActHandler to which the actions can be sent
   \param wm WorldModel which information is used to determine action
   \param ss ServerSettings that contain parameters used by the server
   \param ps PlayerSettings that contain parameters important for the client
   \param strTeamName team name of this player
   \param dVersion version this basicplayer corresponds to
   \param iReconnect integer that defines player number (-1 when new player) */
Player::Player( ActHandler* act, WorldModel *wm, ServerSettings *ss,
      PlayerSettings *ps,
      Formations *fs, char* strTeamName, double dVersion, int iReconnect )

{
  char str[MAX_MSG];

  ACT           = act;
  WM            = wm;
  SS            = ss;
  PS            = ps;
  formations    = fs;
  bContLoop     = true;
  m_iPenaltyNr  = -1;
  WM->setTeamName( strTeamName );
  m_timeLastSay = -5;
  m_objMarkOpp  = OBJECT_ILLEGAL;
  m_actionPrev  = ACT_ILLEGAL;
  dPlayerVersion = dVersion;
  // wait longer as role number increases, to make sure players appear at the
  // field in the correct order
#ifdef WIN32
  Sleep( formations->getPlayerInFormation()*500 );
#else
  poll( 0, 0, formations->getPlayerInFormation()*500 );
#endif

  // create initialisation string
  if( iReconnect != -1 )
    sprintf( str, "(reconnect %s %d)", strTeamName, iReconnect );
  else if( formations->getPlayerType() == PT_GOALKEEPER )
    sprintf( str, "(init %s (version %f) (goalie))", strTeamName, dVersion );
  else
    sprintf( str, "(init %s (version %f))", strTeamName, dVersion );
  ACT->sendMessage( str );
  
}

/*! This is the main loop of the agent. This method calls the update methods
    of the world model after it is indicated that new information has arrived.
    After this, the correct main loop of the player type is called, which
    puts the best soccer command in the queue of the ActHandler. */
void Player::mainLoop( )
{
  char str[MAX_MSG];
  Timing timer;

  // wait for new information from the server
  // cannot say bContLoop=WM->wait... since bContLoop can be changed elsewhere
  if(  WM->waitForNewInformation() == false )
    bContLoop =  false;

  // and set the clang version
  sprintf( str, "(clang (ver 8 8))" );
  ACT->sendMessage( str );

  while( bContLoop )                                 // as long as server alive
  {
    Log.logWithTime( 3, "  start update_all" );
//    Log.setHeader( WM->getCurrentCycle(), WM->getPlayerNumber() );
    Log.setHeader( WM->getCurrentCycle() );
    LogDraw.setHeader( WM->getCurrentCycle() );

    if( WM->updateAll( ) == true )
    {
      timer.restartTime();
      SoccerCommand soc;
      if( ( WM->isPenaltyUs( ) || WM->isPenaltyThem() ) )
        performPenalty();
      else if( WM->getPlayMode() == PM_FROZEN )
        ACT->putCommandInQueue( turnBodyToObject( OBJECT_BALL )  ); 
      else
      {
        switch( formations->getPlayerType( ) )        // determine right loop
        {
          case PT_GOALKEEPER:       soc = goalieMainLoop( );     break;
          case PT_DEFENDER_SWEEPER:
          case PT_DEFENDER_CENTRAL:
          case PT_DEFENDER_WING:    soc = defenderMainLoop( );   break;
          case PT_MIDFIELDER_CENTER:
          case PT_MIDFIELDER_WING:  soc = midfielderMainLoop( ); break;
          case PT_ATTACKER:
          case PT_ATTACKER_WING:    soc = attackerMainLoop( );   break;
          case PT_ILLEGAL:
          default: break;
        }

        if( shallISaySomething(soc) == true )           // shall I communicate
        {
          m_timeLastSay = WM->getCurrentTime();
          char strMsg[MAX_SAY_MSG+1];
          if( WM->getPlayerNumber() == 6 &&
              WM->getBallPos().getX() < - PENALTY_X + 4.0  )
            sayOppAttackerStatus( strMsg );
          else
            sayBallStatus( strMsg );
          if( strlen( strMsg ) != 0 )
            Log.log( 600, "send communication string: %s", strMsg );
          WM->setCommunicationString( strMsg );
        }
      }
      Log.logWithTime( 3, "  determined action; waiting for new info" );
      // directly after see message, will nog get better info, so send commands
      if( WM->getTimeLastSeeMessage() == WM->getCurrentTime() ||
          (SS->getSynchMode() == true && WM->getRecvThink() == true ))
      {
        Log.logWithTime( 3, "  send messages directly" );
        ACT->sendCommands( );
        Log.logWithTime( 3, "  sent messages directly" );
        if( SS->getSynchMode() == true  )
        {
          WM->processRecvThink( false );
          ACT->sendMessageDirect( "(done)" );
        }
      }

    }
    else
      Log.logWithTime( 3, "  HOLE no action determined; waiting for new info");

    if( WM->getCurrentCycle()%(SS->getHalfTime()*SS->getSimulatorStep()) != 0 )
    {
      if( LogDraw.isInLogLevel( 600 ) )
      {
        WM->logDrawInfo( 600 );
      }

      if( LogDraw.isInLogLevel( 601 ) )
        WM->logDrawBallInfo( 601 );

      if( LogDraw.isInLogLevel( 700 ) )
        WM->logCoordInfo( 700 );
    }
 
    Log.logWithTime( 604, "time for action: %f", timer.getElapsedTime()*1000 );
           
    // wait for new information from the server cannot say
    // bContLoop=WM->wait... since bContLoop can be changed elsewhere
    if(  WM->waitForNewInformation() == false )
        bContLoop =  false;
  }

  // shutdow, print hole and number of players seen statistics
  printf("Shutting down player %d\n", WM->getPlayerNumber() );
  printf("   Number of holes: %d (%f)\n", WM->iNrHoles,
                         ((double)WM->iNrHoles/WM->getCurrentCycle())*100 );
  printf("   Teammates seen: %d (%f)\n", WM->iNrTeammatesSeen,
                         ((double)WM->iNrTeammatesSeen/WM->getCurrentCycle()));
  printf("   Opponents seen: %d (%f)\n", WM->iNrOpponentsSeen,
                         ((double)WM->iNrOpponentsSeen/WM->getCurrentCycle()));

}


/*! This is the main decision loop for the goalkeeper. */
SoccerCommand Player::goalieMainLoop( )
{
  return deMeer5_goalie();
}

/*! This is the main decision loop for a defender. */
SoccerCommand Player::defenderMainLoop( )
{
  return deMeer5() ;
}

/*! This is the main decision loop for a midfielder. */
SoccerCommand Player::midfielderMainLoop( )
{
  return deMeer5() ;
}

/*! This is the main decision loop for an agent. */
SoccerCommand Player::attackerMainLoop( )
{
  return deMeer5() ;
}

/*! This method returns the position to move in case of a dead ball situation.
    A dead ball situation occurs when the team can have a free kick, kick in,
    etc. The agent will move to the position behind the ball and when he is
    there will move to the ball again. */
VecPosition Player::getDeadBallPosition(  )
{
  VecPosition pos, posBall = WM->getBallPos();
  VecPosition posAgent = WM->getAgentGlobalPosition();
  double dDist;

  // determine point to move to
  if( WM->isKickInUs()  )
    pos = posBall + VecPosition( -1.5, sign( posBall.getY() )*1.5 );
  else if( WM->isCornerKickUs( ) )
    pos = posBall + VecPosition( 1.5, sign( posBall.getY() ) * 1.5 );
  else if( WM->isFreeKickUs() || WM->isOffsideThem() || WM->isGoalKickUs() ||
           WM->isFreeKickFaultThem() || WM->isBackPassThem() )
    pos = posBall + VecPosition( -1.5, 0.0 );
  else
    return VecPosition( UnknownDoubleValue, UnknownDoubleValue );

  AngDeg      angBall = (posBall-posAgent).getDirection() ;
  ObjectT     obj = WM->getClosestInSetTo( OBJECT_SET_PLAYERS,
                                           WM->getAgentObjectType(), &dDist);
  VecPosition posPlayer = WM->getGlobalPosition( obj );

  // change point when heading towards other player or towards the ball
  if( fabs( angBall - (posPlayer-posAgent).getDirection() ) < 20 &&
      dDist < 6 )
    pos -= VecPosition( 5, 0 );
  if( fabs( angBall -  (pos-posAgent).getDirection()) < 20 )
  {
    angBall = VecPosition::normalizeAngle( angBall - 90 );
    pos = posBall + VecPosition( 1, angBall , POLAR );
  }
  return pos;
}









/*!This method listens for input from the keyboard and when it receives this
   input it converts this input to the associated action. See
   showStringCommands for the possible options. This method is used together
   with the SenseHandler class that sends an alarm to indicate that a new
   command can be sent. This conflicts with the method in this method that
   listens for the user input (fgets) on Linux systems (on Solaris this isn't a
   problem). The only known method is to use the flag SA_RESTART with this
   alarm function, but that does not seem to work under Linux. If each time
   the alarm is sent, this gets function unblocks, it will cause major
   performance problems. This function should not be called when a whole match
   is played! */
void Player::handleStdin( )
{
  char buf[MAX_MSG];

  while( bContLoop )
  {
#ifdef WIN32
    cin.getline( buf, MAX_MSG );
#else
    fgets( buf, MAX_MSG, stdin ); // does unblock with signal !!!!!
#endif
   printf( "after fgets: %s\n", buf );
   executeStringCommand( buf );
  }
}

/*!This method prints the possible commands that can be entered by the user.
   The whole name can be entered to perform the corresponding command, but
   normally only the first character is sufficient. This is indicated by
   putting brackets around the part of the command that is not needed.
   \param out output stream to which the possible commands are printed */
void Player::showStringCommands( ostream& out )
{
  out << "Basic commands:"                                << endl <<
         " a(ctions)"                                     << endl <<
         " c(atch) direction"                             << endl <<
         " cs(lientsettings"                              << endl <<
         " d(ash) power [ times ]"                        << endl <<
         " de(bug) nr_cycles"                             << endl <<
         " g(oto) x y"                                    << endl <<
         " h(elp)"                                        << endl <<
         " i(ntercept) x y"                               << endl <<
         " k(ick) power angle"                            << endl <<
         " ka x y endspeed "                              << endl <<
         " m(ove) x y"                                    << endl <<
         " n(eck) angle"                                  << endl <<
         " o(pponents in cone) width dist"                << endl <<
         " p(redict cycles to) x y"                       << endl <<
         " q(uit)"                                        << endl <<
         " s(ay) message"                                 << endl <<
         " ss(erversettings)"                             << endl <<
         " t(urn) angle"                                  << endl <<
         " v(iewmode) narrow | normal | wide low | high"  << endl <<
         " w(orldmodel)"                                  << endl;
}

/*!This method executes the command that is entered by the user. For the
   possible command look at the method showStringCommands.
   \param str string that is entered by the user
   \return true when command could be executed, false otherwise */
bool Player::executeStringCommand( char *str)
{
  SoccerCommand socCommand;
  int           i;
  double        x, y;

  switch( str[0] )
  {
    case 'a':                                 // actions
      WM->showQueuedCommands();
      break;
    case 'c':                                 // catch dir or cs
      if( strlen(str) > 1 && str[1] == 's' )
      {
        PS->show( cout, ":" );
        break;
      }
      socCommand.makeCommand( CMD_CATCH, Parse::parseFirstInt( &str ) );
      break;
    case 'd':                                 // dash
      socCommand.commandType = CMD_DASH;
      socCommand.dPower      = Parse::parseFirstDouble( &str );
      socCommand.iTimes      = Parse::parseFirstInt   ( &str );
      if( socCommand.iTimes == 0 ) socCommand.iTimes = 1;
      break;
    case 'h':                                // help
      showStringCommands( cout );
      return true;
    case 'k':                                // kick or ka (kick advanced)
      socCommand.commandType = CMD_KICK;
      if( str[1] == 'a' ) // advanced kick
      {
        double x = Parse::parseFirstDouble( &str );
        double y = Parse::parseFirstDouble( &str );
        double e = Parse::parseFirstDouble( &str );
        socCommand = kickTo( VecPosition( x, y), e );
      }
      else
      {
        socCommand.dPower = Parse::parseFirstDouble( &str );
        socCommand.dAngle = Parse::parseFirstDouble( &str );
      }
      break;
    case 'm':                               // move
      socCommand.commandType = CMD_MOVE;
      socCommand.dX          = Parse::parseFirstDouble( &str );
      socCommand.dY          = Parse::parseFirstDouble( &str );
      socCommand.dAngle      = Parse::parseFirstDouble( &str );
      break;
    case 'n':                              // turn_neck
      socCommand.commandType = CMD_TURNNECK;
      socCommand.dAngle      = Parse::parseFirstDouble( &str );
      break;
    case 'o':                              // count nr opp in cone
      x = Parse::parseFirstDouble( &str );
      y = Parse::parseFirstDouble( &str );
      i = WM->getNrInSetInCone( OBJECT_SET_OPPONENTS, x, 
                                WM->getAgentGlobalPosition(),
                                WM->getAgentGlobalPosition() + 
                                VecPosition( y,
                                             WM->getAgentGlobalNeckAngle(), 
                                             POLAR ) );
      printf( "%d opponents\n", i );
      return true;
    case 'p':                              // predict cycles to point
      x = Parse::parseFirstDouble( &str );
      y = Parse::parseFirstDouble( &str );
      i = WM->predictNrCyclesToPoint( WM->getAgentObjectType(),
                                      VecPosition( x, y ) );
      printf( "%d cycles\n", i );
      return true;
    case 'q':                             // quit
      bContLoop = false;
      return true;
    case 's':                             // ss (serversettings) or say
      if( strlen(str) > 1 && str[1] == 's' )
      {
        SS->show( cout, ":" );
        break;
      }
      socCommand.commandType = CMD_SAY;
      Parse::gotoFirstOccurenceOf( ' ', &str );
      Parse::gotoFirstNonSpace( &str );
      strcpy( socCommand.str, str);
      break;
    case 't':                             // turn
      socCommand.commandType = CMD_TURN;
      socCommand.dAngle      = Parse::parseFirstDouble( &str );
      break;
    case 'v':                             // change_view
      socCommand.commandType = CMD_CHANGEVIEW;
      Parse::gotoFirstOccurenceOf(' ', &str );
      Parse::gotoFirstNonSpace( &str );
      socCommand.va          = SoccerTypes::getViewAngleFromStr( str );
      Parse::gotoFirstOccurenceOf(' ', &str );
      Parse::gotoFirstNonSpace( &str );
      socCommand.vq          = SoccerTypes::getViewQualityFromStr( str );
      break;
    case 'w':                            // worldmodel
      WM->show();
      return true;
    default:                             // default: send entered string
      ACT->sendMessage( str );
      return true;
  }
  if( socCommand.commandType != CMD_ILLEGAL ) // when socCommand is set
    ACT->putCommandInQueue( socCommand );     // send it.

  return true;
}

/*!This method can be called in a separate thread (see pthread_create) since
   it returns a void pointer. It is assumed that this function receives a
   BasicPlayer class as argument. The only thing this function does
   is starting the method handleStdin() from the corresponding BasicPlayer
   class that listens to user input from the keyboard. This function is
   necessary since a method from a class cannot be an argument of
   pthread_create.
   \param v void pointer to a BasicPlayer class.
   \return should never return since function handleStdin has infinite loop*/
#ifdef WIN32
DWORD WINAPI stdin_callback( LPVOID v )
#else
void* stdin_callback( void * v )
#endif

{
  Log.log( 1, "Starting to listen for user input" );
  Player* p = (Player*)v;
  p->handleStdin();
  return NULL;
}

/********************** SAY **************************************************/

/*!This method determines whether a player should say something.
   \return bool indicating whether the agent should say a message */
bool Player::shallISaySomething( SoccerCommand socPri )
{
  bool        bReturn;

  bReturn  = ((WM->getCurrentTime() - m_timeLastSay) >= SS->getHearDecay());
  bReturn  &= amIAgentToSaySomething( socPri );
  bReturn  &= (WM->getCurrentCycle() > 0 );

  return bReturn;
}

/*! This method returns a boolean indicating whether I should communicate my
    world model to the other agents.
    \return boolean indicating whether I should communicate my world model. */
bool Player::amIAgentToSaySomething( SoccerCommand socPri )
{
  double  dDist;
  ObjectT obj;

  // get the closest teammate to the ball, if we are not him, we do not
  // communicate since he will
  obj = WM->getClosestInSetTo( OBJECT_SET_TEAMMATES,OBJECT_BALL,&dDist);
  if( dDist < SS->getVisibleDistance() &&
      obj != WM->getAgentObjectType() )
    return false;

  // in the defense, player 6 keeps track of the opponent attacker
  if( WM->getBallPos().getX() < - PENALTY_X + 4.0 &&
      WM->getConfidence( OBJECT_BALL ) > 0.96 &&
      WM->getPlayerNumber() == 6 &&
      WM->getCurrentCycle() % 3 == 0 ) // once very 3 cycles is enough
  {
    Log.log( 600, "player 6 is going to communicate attacker info" );
    return true;
  }

  VecPosition posBallPred;
  WM->predictBallInfoAfterCommand( socPri, &posBallPred );
  VecPosition posAgentPred = WM->predictAgentPosAfterCommand( socPri );
  // in all other cases inform teammates of ball when you have good information
  if( ( WM->getTimeChangeInformation(OBJECT_BALL) == WM->getCurrentTime() &&
          WM->getRelativeDistance( OBJECT_BALL ) < 20.0 &&
          WM->getTimeLastSeen( OBJECT_BALL ) == WM->getCurrentTime() )
        ||
      (
       WM->getRelativeDistance( OBJECT_BALL ) < SS->getVisibleDistance() &&
       WM->getTimeLastSeen( OBJECT_BALL ) == WM->getCurrentTime()  
       )
      ||
      ( // pass ball
       WM->getRelativeDistance( OBJECT_BALL ) < SS->getMaximalKickDist() &&
       posBallPred.getDistanceTo( posAgentPred ) > SS->getMaximalKickDist() 
       )
      )
    return true;

  return false;
}

/*! This method encodes the opponent attacker status in a visual message.
    \return string in which the opponent attacker position is encoded.*/
void Player::sayOppAttackerStatus( char* strMsg )
{
  char    strTmp[MAX_SAY_MSG];

  // fill the first byte with some encoding to indicate the current cycle.
  // The second byte   is the last digit of the cycle added to 'a'.
  sprintf( strMsg, "%c", 'a' + WM->getCurrentCycle()%10   );

  // fill the second byte with information about the offside line.
  // Enter either a value between a-z or A-Z indicating. This gives 52 possible
  // values which correspond with meters on the field. So B means the offside
  // line lies at 27.0.
  int iOffside = (int)WM->getOffsideX();
  if( iOffside < 26 ) // 0..25
    sprintf( strTmp, "%c", 'a' + iOffside );
  else               // 26...
    sprintf( strTmp, "%c", 'A' + max(iOffside - 26, 25) );
  strcat( strMsg, strTmp );

  // find the closest opponent attacker to the penalty spot.
  double dDist ;
  ObjectT objOpp = WM->getClosestInSetTo( OBJECT_SET_OPPONENTS,
         VecPosition(- PITCH_LENGTH/2.0 + 11.0, 0 ), &dDist  ) ;

  if( objOpp == OBJECT_ILLEGAL || dDist >= 20.0 )
  {
    strncpy( strMsg, "", 0 );
    return;
  }

  VecPosition posOpp = WM->getGlobalPosition( objOpp );
  if( fabs( posOpp.getY() ) > 10 )
  {
    strncpy( strMsg, "", 0 );
    return;
  }

  // encode the position of this attacker in the visual message. The
  // player_number is the third byte, then comes the x position in 3 digits (it
  // is assumed this value is always negative), a space and finally the y
  // position in 2 digits. An example of opponent nr. 9 at position
  // (-40.3223,-3.332) is "j403 -33)
  sprintf( strTmp, "%c%d %d", 'a' + SoccerTypes::getIndex( objOpp ) ,
                              (int)(fabs(posOpp.getX())*10),
                              (int)(posOpp.getY()*10));
  strcat( strMsg, strTmp );

  return ;
}

/*! This method creates a string to communicate the ball status. When
    the player just kicks the ball, it is the new velocity of the
    ball, otherwise it is the current velocity.
    \param strMsg will be filled */
void Player::sayBallStatus( char * strMsg  )
{
  VecPosition posBall = WM->getGlobalPosition( OBJECT_BALL );
  VecPosition velBall = WM->getGlobalVelocity( OBJECT_BALL );
  int iDiff = 0;
  SoccerCommand soc = ACT->getPrimaryCommand();

  if( WM->getRelativeDistance( OBJECT_BALL ) < SS->getMaximalKickDist() )
  {
    // if kick and a pass
    if( soc.commandType == CMD_KICK )
    {
      WM->predictBallInfoAfterCommand( soc, &posBall, &velBall );
      VecPosition posAgent = WM->predictAgentPos( 1, 0 );
      if( posBall.getDistanceTo( posAgent ) > SS->getMaximalKickDist() + 0.2 )
        iDiff = 1;
    }

    if( iDiff == 0 )
    {
      posBall = WM->getGlobalPosition( OBJECT_BALL );
      velBall.setVecPosition( 0, 0 );
    }
  }
  Log.log( 600, "create comm. ball after: (%1.2f,%1.2f)(%1.2f,%1.2f) diff %d",
     posBall.getX(), posBall.getY(), velBall.getX(), velBall.getY(), iDiff);
  makeBallInfo( posBall, velBall, iDiff, strMsg );
}

/*! This method is used to create the communicate message for the
    status of the ball, that is its position and velocity is encoded.

    \param VecPosition posBall ball position
    \param VecPosition velBall ball velocity
    \param iDiff time difference corresponding to given ball information
    \strMsg string message in which the ball information is encoded. */
void Player::makeBallInfo( VecPosition posBall, VecPosition velBall, int iDiff,
                            char * strMsg  )
{
  char    strTmp[MAX_SAY_MSG];

  // fill the first byte with some encoding to indicate the next cycle.
  // The second byte is the last digit of the cycle added to 'a'.
  sprintf( strMsg, "%c", 'a' + (WM->getCurrentCycle()+iDiff)%10   );

  // fill the second byte with information about the offside line.
  // Enter either a value between a-z or A-Z indicating. This gives 52 possible
  // values which correspond with meters on the field. So B means the offside
  // line lies at 27.0.
  int iOffside = (int)( WM->getOffsideX( false ) - 1.0 );
  if( iOffside < 26 ) // 0..25
    sprintf( strTmp, "%c", 'a' + max( 0, iOffside) );
  else               // 26...
    sprintf( strTmp, "%c", 'A' + min(iOffside - 26, 25) );
  strcat( strMsg, strTmp );

  // First add al values to a positive interval, since then we don't need
  // another byte for the minus sign. then take one digit at a time
  double x = max(0,min( rint( posBall.getX() + 48.0), 99.0));
  sprintf( strTmp, "%c%c%c%c%c%c%c%c",
     '0' + ((int)( x                          ) % 100 ) / 10 ,
     '0' + ((int)( x                          ) % 100 ) % 10 ,
     '0' + ((int)( rint(posBall.getY() + 34.0)) % 100 ) / 10 ,
     '0' + ((int)( rint(posBall.getY() + 34.0)) % 100 ) % 10 ,
     '0' + ((int)(( velBall.getX() + 2.7) * 10 )) / 10 ,
     '0' + ((int)(( velBall.getX() + 2.7) * 10 )) % 10 ,
     '0' + ((int)(( velBall.getY() + 2.7) * 10 )) / 10 ,
     '0' + ((int)(( velBall.getY() + 2.7) * 10 )) % 10 );
  strcat( strMsg, strTmp );
  Log.log( 6560, "say (%d) %s\n", WM->getPlayerNumber() , strMsg );

  return ;
}

// ------------------------------------------- RoboCap 2D ------------------------------------------------//
int Player::getRegion(ObjectT o){
VecPosition agente = WM->getGlobalPosition(o);
int t = 20;
        
        if((agente.getX()-54)*(agente.getX()-54) + agente.getY()*agente.getY() < t*t)
        return 2;
        else if(((agente.getX()-54)*(agente.getX()-54) + agente.getY()*agente.getY() > t*t) && agente.getY()>0 && agente.getX()>36)
        return 3;
        else if(((agente.getX()-54)*(agente.getX()-54) + agente.getY()*agente.getY() > t*t) && agente.getY()<0 && agente.getX()>36)
        return 4;
        else
        return 1;
        }


VecPosition Player::calculatoque(){

	ObjectT o[] = { OBJECT_TEAMMATE_2, OBJECT_TEAMMATE_3, OBJECT_TEAMMATE_4,
			OBJECT_TEAMMATE_5, OBJECT_TEAMMATE_6, OBJECT_TEAMMATE_7,
			OBJECT_TEAMMATE_8, OBJECT_TEAMMATE_9, OBJECT_TEAMMATE_10,
			OBJECT_TEAMMATE_11 };// vetor de todos os jogadores do seu time
	ObjectT r1[10]; // todos os jogadores que estao na regiao 1
	int c1 = 0; // contador para o numero de jogadores que estao na regiao 1
        for (int i = 0; i < 10; i++) {

		if (getRegion(o[i]) == 1) { // verifica se ta na regiao 1
			r1[c1] = o[i]; // seta os objetos que estao na  regiao 1
			c1++; // quantos jogadores tem na regiao 1
		}
	}
	int numjogadores = c1; // numero de jogadores na regiao 1;

	double aux = 0; // auxiliar para comparar as distancias
	double Pi = 0; // peso da posicao de i
	double tempob[numjogadores]; // tempo de bola do toque
	double tempoad[numjogadores]; // tempo que o adversario leva pra interceptar a bola.
	double d[c1];
	double l[c1];
	double T[c1];
	double dist=10.0;
	double Ad = 0.0;
	int 	indice_jogador = 0;
	for (int i = 0; i < c1; i++) {
		d[i] = WM->getRelativeDistance(r1[i]);
	}
	//for (int i = 0; i < c1; i++) {
	//	l[i] = makeLineFromTwoPoints(WM->getAgentGlobalPosition(),WM->getGlobalPosition(r1[i]));
	//}
	//VecPosition jogRoubaBola = WM->getGlobalPosition(WM->getFastestInSetTo(  OBJECT_SET_OPPONENTS, OBJECT_BALL
	//));
	//makeLineFromPositionAndAngle( jogRoubaBola, 90.0 );

	// getDistanceWithPoint();
	for (int i = 0; i < numjogadores; i++) {
		//tempob[i] = WM->getBallSpeed() / d[i];
		//tempoad[i];
		if (d[i] <= dist) // if para d menores que a d ideal
			Pi = d[i] / dist;

		else if (d[i] > dist) // if para d maiores que a d ideal
			Pi = (2 * dist - d[i]) / dist;

		if (5.0<7.0) // tempo de bola < tempo de roubada de bola
			Ad = 0.0;
		else
			Ad = 1.0;

		T[i] = Pi - Ad; // retorna o peso ideal(o maior) 
		if (T[i] >= aux) { // comparacao dos pesos de pi e aux
			aux = T[i]; // recebe o maior peso.
			indice_jogador = i;
		}
	}
	return WM->getGlobalPosition(r1[indice_jogador]);


}

VecPosition Player::calculatoque3(){

	ObjectT o[] = { OBJECT_TEAMMATE_2, OBJECT_TEAMMATE_3, OBJECT_TEAMMATE_4,
			OBJECT_TEAMMATE_5, OBJECT_TEAMMATE_6, OBJECT_TEAMMATE_7,
			OBJECT_TEAMMATE_8, OBJECT_TEAMMATE_9, OBJECT_TEAMMATE_10,
			OBJECT_TEAMMATE_11 };// vetor de todos os jogadores do seu time
	ObjectT r1[10]; // todos os jogadores que estao na regiao 1
	int c1 = 0; // contador para o numero de jogadores que estao na regiao 1
        for (int i = 0; i < 10; i++) {

		if (getRegion(o[i]) == 3) { // verifica se ta na regiao 1
			r1[c1] = o[i]; // seta os objetos que estao na  regiao 1
			c1++; // quantos jogadores tem na regiao 1
		}
	}
	int numjogadores = c1; // numero de jogadores na regiao 1;

	double aux = 0; // auxiliar para comparar as distancias
	double Pi = 0; // peso da posicao de i
	double tempob[numjogadores]; // tempo de bola do toque
	double tempoad[numjogadores]; // tempo que o adversario leva pra interceptar a bola.
	double d[c1];
	double l[c1];
	double T[c1];
	double dist=10.0;
	double Ad = 0.0;
	int 	indice_jogador = 0;
	for (int i = 0; i < c1; i++) {
		d[i] = WM->getRelativeDistance(r1[i]);
	}
	//for (int i = 0; i < c1; i++) {
	//	l[i] = makeLineFromTwoPoints(WM->getAgentGlobalPosition(),WM->getGlobalPosition(r1[i]));
	//}
	//VecPosition jogRoubaBola = WM->getGlobalPosition(WM->getFastestInSetTo(  OBJECT_SET_OPPONENTS, OBJECT_BALL
	//));
	//makeLineFromPositionAndAngle( jogRoubaBola, 90.0 );

	// getDistanceWithPoint();
	for (int i = 0; i < numjogadores; i++) {
		//tempob[i] = WM->getBallSpeed() / d[i];
		//tempoad[i];
		if (d[i] <= dist) // if para d menores que a d ideal
			Pi = d[i] / dist;

		else if (d[i] > dist) // if para d maiores que a d ideal
			Pi = (2 * dist - d[i]) / dist;

		if (5.0<7.0) // tempo de bola < tempo de roubada de bola
			Ad = 0.0;
		else
			Ad = 1.0;

		T[i] = Pi - Ad; // retorna o peso ideal(o maior) 
		if (T[i] >= aux) { // comparacao dos pesos de pi e aux
			aux = T[i]; // recebe o maior peso.
			indice_jogador = i;
		}
	}
	return WM->getGlobalPosition(r1[indice_jogador]);


}

VecPosition Player::calculatoque4(){

	ObjectT o[] = { OBJECT_TEAMMATE_2, OBJECT_TEAMMATE_3, OBJECT_TEAMMATE_4,
			OBJECT_TEAMMATE_5, OBJECT_TEAMMATE_6, OBJECT_TEAMMATE_7,
			OBJECT_TEAMMATE_8, OBJECT_TEAMMATE_9, OBJECT_TEAMMATE_10,
			OBJECT_TEAMMATE_11 };// vetor de todos os jogadores do seu time
	ObjectT r1[10]; // todos os jogadores que estao na regiao 1
	int c1 = 0; // contador para o numero de jogadores que estao na regiao 1
        for (int i = 0; i < 10; i++) {

		if (getRegion(o[i]) == 4) { // verifica se ta na regiao 1
			r1[c1] = o[i]; // seta os objetos que estao na  regiao 1
			c1++; // quantos jogadores tem na regiao 1
		}
	}
	int numjogadores = c1; // numero de jogadores na regiao 1;

	double aux = 0; // auxiliar para comparar as distancias
	double Pi = 0; // peso da posicao de i
	double tempob[numjogadores]; // tempo de bola do toque
	double tempoad[numjogadores]; // tempo que o adversario leva pra interceptar a bola.
	double d[c1];
	double l[c1];
	double T[c1];
	double dist=10.0;
	double Ad = 0.0;
	int 	indice_jogador = 0;
	for (int i = 0; i < c1; i++) {
		d[i] = WM->getRelativeDistance(r1[i]);
	}
	//for (int i = 0; i < c1; i++) {
	//	l[i] = makeLineFromTwoPoints(WM->getAgentGlobalPosition(),WM->getGlobalPosition(r1[i]));
	//}
	//VecPosition jogRoubaBola = WM->getGlobalPosition(WM->getFastestInSetTo(  OBJECT_SET_OPPONENTS, OBJECT_BALL
	//));
	//makeLineFromPositionAndAngle( jogRoubaBola, 90.0 );

	// getDistanceWithPoint();
	for (int i = 0; i < numjogadores; i++) {
		//tempob[i] = WM->getBallSpeed() / d[i];
		//tempoad[i];
		if (d[i] <= dist) // if para d menores que a d ideal
			Pi = d[i] / dist;

		else if (d[i] > dist) // if para d maiores que a d ideal
			Pi = (2 * dist - d[i]) / dist;

		if (5.0<7.0) // tempo de bola < tempo de roubada de bola
			Ad = 0.0;
		else
			Ad = 1.0;

		T[i] = Pi - Ad; // retorna o peso ideal(o maior) 
		if (T[i] >= aux) { // comparacao dos pesos de pi e aux
			aux = T[i]; // recebe o maior peso.
			indice_jogador = i;
		}
	}
	return WM->getGlobalPosition(r1[indice_jogador]);


}
VecPosition Player::calculatoque2(){

	ObjectT o[] = { OBJECT_TEAMMATE_2, OBJECT_TEAMMATE_3, OBJECT_TEAMMATE_4,
			OBJECT_TEAMMATE_5, OBJECT_TEAMMATE_6, OBJECT_TEAMMATE_7,
			OBJECT_TEAMMATE_8, OBJECT_TEAMMATE_9, OBJECT_TEAMMATE_10,
			OBJECT_TEAMMATE_11 };// vetor de todos os jogadores do seu time
	ObjectT r1[10]; // todos os jogadores que estao na regiao 1
	int c1 = 0; // contador para o numero de jogadores que estao na regiao 1
        for (int i = 0; i < 10; i++) {

		if (getRegion(o[i]) == 2) { // verifica se ta na regiao 1
			r1[c1] = o[i]; // seta os objetos que estao na  regiao 1
			c1++; // quantos jogadores tem na regiao 1
		}
	}
	int numjogadores = c1; // numero de jogadores na regiao 1;

	double aux = 0; // auxiliar para comparar as distancias
	double Pi = 0; // peso da posicao de i
	double tempob[numjogadores]; // tempo de bola do toque
	double tempoad[numjogadores]; // tempo que o adversario leva pra interceptar a bola.
	double d[c1];
	double l[c1];
	double T[c1];
	double dist=10.0;
	double Ad = 0.0;
	int 	indice_jogador = 0;
	for (int i = 0; i < c1; i++) {
		d[i] = WM->getRelativeDistance(r1[i]);
	}
	//for (int i = 0; i < c1; i++) {
	//	l[i] = makeLineFromTwoPoints(WM->getAgentGlobalPosition(),WM->getGlobalPosition(r1[i]));
	//}
	//VecPosition jogRoubaBola = WM->getGlobalPosition(WM->getFastestInSetTo(  OBJECT_SET_OPPONENTS, OBJECT_BALL
	//));
	//makeLineFromPositionAndAngle( jogRoubaBola, 90.0 );

	// getDistanceWithPoint();
	for (int i = 0; i < numjogadores; i++) {
		//tempob[i] = WM->getBallSpeed() / d[i];
		//tempoad[i];
		if (d[i] <= dist) // if para d menores que a d ideal
			Pi = d[i] / dist;

		else if (d[i] > dist) // if para d maiores que a d ideal
			Pi = (2 * dist - d[i]) / dist;

		if (5.0<7.0) // tempo de bola < tempo de roubada de bola
			Ad = 0.0;
		else
			Ad = 1.0;

		T[i] = Pi - Ad; // retorna o peso ideal(o maior) 
		if (T[i] >= aux) { // comparacao dos pesos de pi e aux
			aux = T[i]; // recebe o maior peso.
			indice_jogador = i;
		}
	}
	return WM->getGlobalPosition(r1[indice_jogador]);


}

SoccerCommand Player::correparalateral(){
SoccerCommand soc;
VecPosition agente = WM->getAgentGlobalPosition(); // pega a posição global do agente
VecPosition pos1 = (0,-32); // declaração da posição para comparar a distancia do jogador
VecPosition pos2 = (0,+32); // declaração da posição para comparar a distancia do jogador
VecPosition pos3 = (agente.getX(),+32);
VecPosition pos4 = (agente.getX(),-32);
        if(agente.getDistanceTo(pos1) > agente.getDistanceTo(pos2) && pos1.getDistanceTo(pos2)) {// comparando qual a maior distancia do jogador as posições
        soc = dribble(45,DRIBBLE_WITHBALL); // corre para a lateral 
        return soc;        
        }       
       if(agente.getDistanceTo(pos1) < agente.getDistanceTo(pos2) && pos1.getDistanceTo(pos2))  {
        soc = dribble(-45,DRIBBLE_WITHBALL); 
        return soc;    
        }   
                
                if(pos3.getDistanceTo(pos2) <= 2){ // distancia do agente até a lateral for menor que 2 corre reto
                soc = dribble(0, DRIBBLE_WITHBALL);
                return soc;
                }
                if(pos4.getDistanceTo(pos1) <=2){
                soc = dribble(0, DRIBBLE_WITHBALL);
                return soc;
                }
}

SoccerCommand Player::bolaparadanossa(){
SoccerCommand soc;
int iTmp;
double dist = 5;
ObjectT agente = WM->getAgentObjectType();
 VecPosition posmelhorpasse = WM->getGlobalPosition(WM->getClosestInSetTo(OBJECT_SET_TEAMMATES,agente,&dist,-1.0));
                        if(WM->getFastestInSetTo( OBJECT_SET_TEAMMATES, OBJECT_BALL, &iTmp )== WM->getAgentObjectType()){ // sou o mais rapido, posso ir.
                                if(WM->isBallKickable() && WM->getAgentGlobalPosition().getDistanceTo(WM->getBallPos())<0.6){
                                soc = directPass(posmelhorpasse,PASS_NORMAL);
                                ACT->putCommandInQueue(soc);
                                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL,soc));
                                
                                }
                                else{ // a bola nao e chutavel
                                soc = moveToPos(WM->getBallPos(),PS->getPlayerWhenToTurnAngle());
                                ACT->putCommandInQueue(soc);
                                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL,soc));
                                }
                        }
                        else{
                                soc = moveToPos(WM->getStrategicPosition(),PS->getPlayerWhenToTurnAngle());//VAI PARA POSIÇÂO ESTRATEGICA
                                ACT->putCommandInQueue(soc);
                                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL,soc));
                            }
                            return soc;
          }

bool Player::temLadrao(){
int distancia=0;
VecPosition posAgent = WM->getAgentGlobalPosition();
ObjectT agente = WM->getAgentObjectType(); // Objeto agente
        if(posAgent.getX()<20)
        distancia =5;
        else if(posAgent.getX()<40)
        distancia = 4;
        else
        distancia = 3;
        
        if(WM->isEmptySpace(agente,360,distancia)) // Se o espaço estiver vazio em um raio de 5m do jogador
        return false;
        else
        return true;
        }
        
        
bool Player:: verificaLonge(){
int z = 10;
VecPosition agente = WM->getAgentGlobalPosition();
VecPosition posBall = WM->getBallPos();
        if(agente.getDistanceTo(posBall)>z)
        return true;
        else
        return false;
        
        }
        
bool Player:: direcaolivre(VecPosition posGol) //Verifica se a direcao esta livre para chute
{
	VecPosition minhaPosicao = WM->getAgentGlobalPosition();
	int numeroInimigos = WM->getNrInSetInCone(OBJECT_SET_OPPONENTS,2.0,minhaPosicao,posGol);
	if(numeroInimigos <= 1)
		return true;
	else
		return false;
}

VecPosition Player:: posGol() //verifica melhor posicao para o chute
{
	VecPosition posicaoGoleiro = WM->getGlobalPosition(OBJECT_OPPONENT_GOALIE);
	//52,-5 . 52,5.
	if((10 - posicaoGoleiro.getY()) < 10)
		return VecPosition(52,-5);
	else
		return VecPosition(52,5);
}

bool Player:: verificaRoubar(int iTmp)//Verifica se sou o jogador mais proximo da bola
{
	if(WM->getFastestInSetTo( OBJECT_SET_TEAMMATES, OBJECT_BALL, &iTmp)   //Sou o jogador mais proximo da bola
              == WM->getAgentObjectType()  && !WM->isDeadBallThem())
		return true;
	else
		return false;
}

SoccerCommand Player:: vaiLinhadeFundo()//move o jogador para a linha de fundo
{
	VecPosition minhaPosicao = WM->getAgentGlobalPosition();
	SoccerCommand soc;
	if(minhaPosicao.getX() < 54)
		soc = moveToPos(VecPosition(54,minhaPosicao.getY()),PS->getPlayerWhenToTurnAngle());
	else
		soc = SoccerCommand(CMD_TURNNECK,0.0);
	return soc;
}

bool Player:: alguemjuntocomadv(){
        double dist = 5;
        ObjectT jogmaispertoadv = WM->getClosestRelativeInSet(OBJECT_SET_OPPONENTS,&dist);
        VecPosition posjogmaispertoadv = WM->getGlobalPosition(jogmaispertoadv);
        VecPosition opmaisperto = WM->getPosClosestOpponentTo(&dist ,jogmaispertoadv);
        if(opmaisperto.getDistanceTo(posjogmaispertoadv) <5)
        return true;
        else
        return false;
        }

VecPosition Player:: procuraradvdesmarcado(){
        double dist = 5;
        ObjectT jogmaispertoadv = WM->getClosestRelativeInSet(OBJECT_SET_OPPONENTS,&dist);
        VecPosition posjogmaispertoadv = WM->getGlobalPosition(jogmaispertoadv);
        VecPosition opmaisperto = WM->getPosClosestOpponentTo(&dist ,jogmaispertoadv);
        if(opmaisperto.getDistanceTo(posjogmaispertoadv) >5)
        return posjogmaispertoadv;
        else
        return 0;
        } 
        
        
//Caso o jogador oponente consiga interceptar a bola returna 1,
//caso contrário returna 0(este valor será subtraido no peso Geral)
//(x1,y1)Coord. Jogador a tocar a bola; (x2,y2)Coord. Jogador a receber a bola;
int Player::tempoInterceptacao(double x1, double y1, double x2, double y2)
{
    double a, b, b2,x3,y3,pontx, ponty, dist_mate, dist_opponent=100, tempoad_aux=50,tempob_aux=0,tempob, tempoad, veloc_b=2.7, veloc_ad=1.0;
    ObjectT Opponents[10] ={OBJECT_OPPONENT_2,OBJECT_OPPONENT_3,OBJECT_OPPONENT_4,OBJECT_OPPONENT_5,OBJECT_OPPONENT_6,OBJECT_OPPONENT_7,OBJECT_OPPONENT_8,OBJECT_OPPONENT_9,OBJECT_OPPONENT_10,OBJECT_OPPONENT_11};

    if(x1==x2)  //Corrige erro de divisão por 0, pois nao e possivel definir eq. da reta
        x2 += 0.001; //Para deixar o x1 diferente do x2.

    if(y1==y2) //Corrige erro de divisão por 0, pois a componente 'a' da eq resulta em 0 e impossibilitando calcular seu inverso para o calculo da perpendicular
        y2 += 0.001; //Para deixar o y1 diferente do y2

    a = (y2-y1)/(x2-x1); //Encontrando a componente 'a' da eq.
    b = y1 - a*x1; //Encontrando a componente 'b' da eq.
    //Obtem-se a equação reduzida da reta (y=ax+b)


    //Verificando qual é o jogador oponente mais rápido ao ponto de interceptação.
    for(int i = 0; i < 11; i++)
    {
        //(x3,y3)Coord. Jogador a roubar a bola
        x3 = WM->getGlobalPosition(Opponents[i]).getX();
        y3 = WM->getGlobalPosition(Opponents[i]).getY();

        b2 = y3+1/a*x3; //Encontrando a componente 'b2' da eq. da reta perpendicular formada pelo jogador oponente e ponto de interceptação

        pontx = (b-b2)/((-1/a)-a); //Ponto x que as retas se interceptam
        ponty = (a*pontx) +b; //ponto y onde as retas se interceptam

        dist_mate = sqrt(pow(pontx-x1,2)+pow(ponty-y1,2)); //Distância do jogador que irá tocar até o ponto de interceptação.
        dist_opponent = sqrt(pow(pontx-x3,2)+pow(ponty-y3,2)); //Distâcia do oponente até o ponto de interceptação.


        if(x1<x2)  //Verificar se a componente x do jogador a tocar a bola é menor que a do companheiro a receber a bola
        {
            if(pontx>x1 && pontx<x2)  //Se o ponto de interceptação estiver no segmento de reta(trajetória da bola)
            {
                tempob = dist_mate/veloc_b; //Calcula o tempo que a bola gasta para chegar ao ponto de interceptação
                tempoad = dist_opponent/veloc_ad; //Calcula o tempo que o oponente gasta para chegar ao ponto de interceptação

            }
            //Se o ponto de interceptação não estiver entre o intervalo indicado,
            //o jogador só consegue roubar se estiver depois do jogador a receber,
            //caso contrário não é possível.
            else
            {
                //Se o jogador oponente estiver depois do jogador a receber a bola,
                //a distância não será mais com relação ao ponto de interceptação,
                //mas sim com relação ao jogador a receber a bola.
                if(pontx>x2)
                {
                    dist_opponent = sqrt(pow(x2-x3,2)+pow(y2-y3,2));
                    dist_mate = sqrt(pow(x2-x1,2)+pow(y2-y1,2));

                    tempob = dist_mate/veloc_b;
                    tempoad = dist_opponent/veloc_ad;
                }
                else
                {
                    tempoad = 100; //Este adversário não consegue roubar a bola por estar atrás do jogador a tocar a bola
                }

            }
        }
        else
        {
            if(pontx<x1 && pontx>x2)
            {
                tempob = dist_mate/veloc_b;
                tempoad = dist_opponent/veloc_ad;

            }
            else
            {
                if(pontx<x2)
                {
                    dist_opponent = sqrt(pow(x2-x3,2)+pow(y2-y3,2));
                    dist_mate = sqrt(pow(x2-x1,2)+pow(y2-y1,2));

                    tempob = dist_mate/veloc_b;
                    tempoad = dist_opponent/veloc_ad;
                }
                else
                {
                    tempoad = 100;
                }
            }
        }

        //Verifica o adversário que está mais perto pra roubar a bola.
        if(tempoad<tempoad_aux)
        {
            tempoad_aux = tempoad;
            tempob_aux = tempob;
        }
    }//Fecha o for


    if(tempob_aux<tempoad_aux)
        return 0;
    else
        return 1;

} //Finaliza o método tempoInterceptacao

//Retorna o peso com relação a densidade de jogadores oponentes que estão no triângulo
//formado pelo jogador a receber a bola e as traves do gol.
//(x1,y1)Coord. Jogador a receber a bola; (x2,y2)Coord. da trave de cima;(x3,y3)Coord. da trave de baixo
double Player::densidadeOponentes(double x1, double y1, double x2, double y2, double x3, double y3 )
{
    ObjectT Opponents[10] =     {OBJECT_OPPONENT_2,OBJECT_OPPONENT_3,OBJECT_OPPONENT_4,OBJECT_OPPONENT_5,OBJECT_OPPONENT_6,OBJECT_OPPONENT_7,OBJECT_OPPONENT_8,OBJECT_OPPONENT_9,OBJECT_OPPONENT_10,OBJECT_OPPONENT_11};
    double xmax,xmin,xa,ymax,ymin,ya,xb,yb,xc,yc,beta;
    int nad=0,i=0; //Número de adversários no triângulo;
    int N = 4; //Numero máximo de oponentes no triângulo
    //Multiplica a componente y por -1 devido o eixo y do campo ser invertido.
    y1 = y1*-1;
    y2 = y2*-1;
    y3 = y3*-1;

    xmax= xmin= xa = x1;
    ymax= ymin= ya = y1;
    xb = x2;
    if (xb>xmax) xmax=xb;
    else if (xb<xmin)	xmin= xb;
    yb = y2;
    if (yb>ymax) ymax=yb;
    else if (yb<ymin) ymin= yb;
    xc = x3;
    if (xc>xmax) xmax=xc;
    else if (xc<xmin) xmin= xc;
    yc = y3;
    if (yc>xmax) ymax=yc;
    else if (yc<xmin) ymin= yc;
    beta = ya*(xc-xb)+yb*(xa-xc)+yc*(xb-xa);

    //Verifica quais dos 11 oponentes está no triângulo
    for(i = 0; i < 11; i++)
    {
        //Se o jogador oponente estiver no triângulo incrementa nad.
        if (TestaPonto(WM->getGlobalPosition(Opponents[i]).getX(), WM->getGlobalPosition(Opponents[i]).getY()))
            nad++;

    }
    //Retorna o peso de M
    if (nad > N)
        return 0;
    else
        return 1 - (nad/N);
} //Finaliza o método densidadeOponentes

//Verifica se o jogador oponente está no triângulo e retorna 1, caso contrário retorna 0.
int Player::TestaPonto(double xe, double ye)
{
    double xmax,xmin,ymax,ymin,beta,yb,ya,xc,yc,xa,xb;
    //Multiplica a componente y por -1 devido o eixo y do campo ser invertido.
    ye = ye*-1;
    if ((xe>xmax)||(xe<xmin)||(ye>ymax)||(ye<ymin))
        return 0;
    if (beta > 0)
    {
        if ((yb*(xe-xc)+yc*(xb-xe)+ye*(xc-xb)) < 0)
            return 0;
        if ((ya*(xc-xe)+yc*(xe-xa)+ye*(xa-xc)) < 0)
            return 0;
        if ((ya*(xe-xb)+yb*(xa-xe)+ye*(xb-xa)) < 0)
            return 0;
    }
    else
    {
        if ((yb*(xe-xc)+yc*(xb-xe)+ye*(xc-xb)) > 0)
            return 0;
        if ((ya*(xc-xe)+yc*(xe-xa)+ye*(xa-xc)) > 0)
            return 0;
        if ((ya*(xe-xb)+yb*(xa-xe)+ye*(xb-xa)) > 0)
            return 0;
    }
    return 1;
} //Finaliza o método densidadeOponentes

//Retorna um VecPosition do melhor jogador que está na REGIÃO 2 para tocar a bola,
//recebe como parametro o jogador a tocar a bola
VecPosition Player::melhorJogadorRegiao2 (ObjectT Companheiro)
{
    ObjectT mates[10]; //Vetor de companheiros que estão na região 2
    ObjectT Teammates[10] = {OBJECT_TEAMMATE_2,OBJECT_TEAMMATE_3,OBJECT_TEAMMATE_4,OBJECT_TEAMMATE_5,OBJECT_TEAMMATE_6,OBJECT_TEAMMATE_7,OBJECT_TEAMMATE_8,OBJECT_TEAMMATE_9,OBJECT_TEAMMATE_10,OBJECT_TEAMMATE_11};
    int number_mates = 0; // Numero de companheiros na região 2
    int indice_jogador; //Indice do melhor peso de toque, com isso obter a posição desse jogador

    double T[10]; //T > Peso do toque
    double aux=0;
    double M=0; //M > Peso em relação ao número de adversários no triângulo formado pelo jogador e as traves
    double ad=0; //Ad >  Peso de posição dos adversários com relação a um adversário interceptar a bola

    // Verifica quais companheiros estão na região 2, estes jogadores são colocados no vetor de objetos;
    for(int i=0; i<11; i++)
    {

        if(getRegion(Teammates[i]) == 2)
        {
            mates[number_mates] = Teammates[i];
            number_mates++;
        }
    }

    //Irá verificar o peso de todos os jogadores que estão na Região 2
    for (int i = 0; i < number_mates; i++)
    {

        M = densidadeOponentes(WM->getGlobalPosition(mates[i]).getX(),WM->getGlobalPosition(mates[i]).getY(), //Jogador a receber a bola
                               WM->getGlobalPosition(OBJECT_FLAG_G_R_T).getX(),WM->getGlobalPosition(OBJECT_FLAG_G_R_T).getY(), //Trave de cima
                               WM->getGlobalPosition(OBJECT_FLAG_G_R_B).getX(),WM->getGlobalPosition(OBJECT_FLAG_G_R_B).getY()); //Trave de baixo


        ad = tempoInterceptacao(WM->getGlobalPosition(Companheiro).getX(),WM->getGlobalPosition(Companheiro).getY(),
                                WM->getGlobalPosition(mates[i]).getX(),WM->getGlobalPosition(mates[i]).getY());

        T[i] =  M - ad;

        if (T[i]>=aux)
        {
            aux = T[i]; //Double auxiliar recebe o valor do maior peso
            indice_jogador = i; //Obtem o indice do vetor de companheiros, para saber qual é o jogador com melhor peso
        }


    } //Fecha o for

    VecPosition pos_jogador = WM->getGlobalPosition(mates[indice_jogador]); // Posição do melhor jogador
    return pos_jogador; //Retorna a posição global do melhor jogador para tocar a bola.
} //Finaliza o método melhorJogadorRegiao2       


//---------------------------------------------------RoboCap2D-------------------------------------------------//

/*! This method is called when a penalty kick has to be taken (for both the 
  goalkeeper as the player who has to take the penalty. */
void Player::performPenalty( )
{
  VecPosition   pos;
  int           iSide = ( WM->getSide() == WM->getSidePenalty() ) ? -1 : 1;
  VecPosition   posPenalty( iSide*(52.5 - SS->getPenDistX()), 0.0 );
  VecPosition   posAgent = WM->getAgentGlobalPosition();
  AngDeg        angBody  = WM->getAgentGlobalBodyAngle();

  SoccerCommand soc(CMD_ILLEGAL);
  static PlayModeT pmPrev = PM_ILLEGAL;

  // raise number of penalties by one when a penalty is taken
  if(
    ( WM->getSide() == SIDE_LEFT && 
      pmPrev != PM_PENALTY_SETUP_LEFT && 
      WM->getPlayMode() == PM_PENALTY_SETUP_LEFT )
    ||
    ( WM->getSide() == SIDE_RIGHT && 
      pmPrev != PM_PENALTY_SETUP_RIGHT && 
      WM->getPlayMode() == PM_PENALTY_SETUP_RIGHT ) )
   m_iPenaltyNr++;

  // start with player 11 and go downwards with each new penalty
  // if we take penalty 
  if( WM->isPenaltyUs() && WM->getPlayerNumber() == (11 - (m_iPenaltyNr % 11)))
  {
     if( WM->getPlayMode() == PM_PENALTY_SETUP_LEFT ||
         WM->getPlayMode() == PM_PENALTY_SETUP_RIGHT )
     {
       pos = posPenalty - VecPosition( iSide*2.0, 0 );
       if( fabs( posAgent.getX() ) > fabs( pos.getX() ) )
         pos = posPenalty;
       if( pos.getDistanceTo( posAgent ) < 0.6 )
       {
         pos = posPenalty;
         if(  fabs( VecPosition::normalizeAngle( 
                   (pos-posAgent).getDirection() - angBody ) ) > 20 )
           soc = turnBodyToPoint( pos );
       }
       //  pos = WM->getAgentGlobalPosition();
     }
     else if( ( WM->getPlayMode() == PM_PENALTY_READY_LEFT ||
                WM->getPlayMode() == PM_PENALTY_READY_RIGHT ||
                WM->getPlayMode() == PM_PENALTY_TAKEN_LEFT ||
                WM->getPlayMode() == PM_PENALTY_TAKEN_RIGHT 
                )
              && WM->isBallKickable() )
     {
       soc = kickTo(VecPosition(iSide*52.5,((drand48()<0.5)?1:-1)*5.9 ),2.7);
     }
     else
       pos = posPenalty;
  }
  else if( formations->getPlayerType() == PT_GOALKEEPER )
  {
    if( WM->getAgentViewAngle() != VA_NARROW )
      ACT->putCommandInQueue( 
		 SoccerCommand( CMD_CHANGEVIEW, VA_NARROW, VQ_HIGH ));

    // is penalty them, stop it, otherwise go to outside field
    pos = posPenalty;
    if( WM->isPenaltyThem( ) )
    {
      pos = VecPosition( iSide*(52.5 - 2.0), 0.0 );
      if( SS->getPenAllowMultKicks() == false )
      {
	if( WM->getPlayMode() == PM_PENALTY_TAKEN_LEFT ||
	    WM->getPlayMode() == PM_PENALTY_TAKEN_RIGHT )
	{
	  if( WM->isBallCatchable( ) )
	    soc = catchBall();
	  else
	    soc = intercept( true );
	}
      }
      else if( pos.getDistanceTo( posAgent ) < 1.0 )
        soc = turnBodyToPoint( VecPosition( 0,0) ) ;
      else
        soc = moveToPos( pos, 25 );
    }
    else
      pos.setVecPosition( iSide * ( PITCH_LENGTH/2.0 + 2 ) , 25 );
  }
  else
  {
    pos = VecPosition( 5.0,
                       VecPosition::normalizeAngle( 
                         iSide*(50 + 20*WM->getPlayerNumber())),
                       POLAR );
  }


  if( soc.isIllegal() && 
      WM->getAgentGlobalPosition().getDistanceTo( pos ) < 0.8 )
  {
    soc = turnBodyToPoint( posPenalty  );    
  }
  else if( soc.isIllegal() )
  {
    soc = moveToPos( pos, 10);
  }
  if( WM->getAgentStamina().getStamina() < 
      SS->getRecoverDecThr()*SS->getStaminaMax() + 500 &&
    soc.commandType == CMD_DASH)
    soc.dPower = 0;
  
  ACT->putCommandInQueue( soc );
  ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );

  pmPrev = WM->getPlayMode();
}
