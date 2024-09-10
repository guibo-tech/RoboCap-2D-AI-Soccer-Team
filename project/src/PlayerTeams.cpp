
#include "Player.h"

// TIME RoboCap 2D

SoccerCommand Player::deMeer5(  )
{

SoccerCommand soc(CMD_ILLEGAL);
VecPosition   posAgent = WM->getAgentGlobalPosition();
VecPosition   posBall  = WM->getBallPos();
int           iTmp;
ObjectT meuPlayer = WM->getAgentObjectType();
ObjectT meuMaisPerto = WM->getClosestInSetTo(OBJECT_SET_TEAMMATES_NO_GOALIE,meuPlayer);
VecPosition posPerto = WM->getGlobalPosition(meuMaisPerto);
ObjectT agente = WM->getAgentObjectType();
double dist = 5.0;


  if( WM->isBeforeKickOff( ) )
  {
    if( WM->isKickOffUs( ) && WM->getPlayerNumber() == 9 ) // 9 takes kick
    {
      if( WM->isBallKickable())
      {
       
        soc = directPass(posPerto,PASS_FAST);
        ACT->putCommandInQueue(soc);
        ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL,soc));
        Log.log( 100, "take kick off" );
        return soc;
      }
      else
      {
        soc = intercept( false );
        Log.log( 100, "move to ball to take kick-off" );
      }
        ACT->putCommandInQueue( soc );
        ACT->putCommandInQueue( turnNeckToObject( meuMaisPerto, soc ) );
        return soc;
    }
    if( formations->getFormation() != FT_INITIAL || // not in kickoff formation
        posAgent.getDistanceTo( WM->getStrategicPosition() ) > 2.0 )
    {
        formations->setFormation( FT_INITIAL );       // go to kick_off formation
        ACT->putCommandInQueue( soc=teleportToPos( WM->getStrategicPosition() ));
    }
    else                                            // else turn to center
    {
        ACT->putCommandInQueue( soc=turnBodyToPoint( VecPosition( 0, 0 ), 0 ) );
        ACT->putCommandInQueue( alignNeckWithBody( ) );
    }
  }  
  else // JOGO COMECOU
  {
    formations->setFormation( FT_433_OFFENSIVE );
    soc.commandType = CMD_ILLEGAL;
    if( WM->getConfidence( OBJECT_BALL ) < PS->getBallConfThr() )
    {
      ACT->putCommandInQueue( soc = searchBall() );   // if ball pos unknown
      ACT->putCommandInQueue( alignNeckWithBody( ) ); // search for it
    } 
    if(WM->isDeadBallUs() || WM->isDeadBallThem()){ // BOLA PARADA
            if(WM->isDeadBallUs()){  //  BOLA PARADA NOSSA
                if(WM->isFreeKickUs()){ // FALTA NOSSA
                soc = bolaparadanossa();
                }
                if(WM->isCornerKickUs()){ // ESCANTEIO NOSSO
                soc = bolaparadanossa();
                }
                if(WM->isOffsideUs()){ // IMPEDIMENTO NOSSO
                soc = bolaparadanossa();
                }
                if(WM->isKickInUs()){ //
                soc = bolaparadanossa();
                }
                if(WM->isFreeKickFaultUs()){
                soc = bolaparadanossa();
                }
                if(WM->isFoulChargeUs()){
                soc = bolaparadanossa();
                }
                if(WM->isKickOffUs()){
                soc = bolaparadanossa();
                }
                if(WM->isBackPassUs()){
                soc = bolaparadanossa();
                }
                if(WM->isGoalKickUs()){
                
                }
                if(WM->isPenaltyUs()){
                        if(WM->getPlayerNumber()==9){
                                if(WM->isBallKickable()){
                                soc = kickTo(posGol(),2.0);
                                ACT->putCommandInQueue(soc);
                                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL,soc));
                                }
                                else{
                                soc = intercept(false);
                                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL,soc));
                                }
                        }
                        else{
                                soc = moveToPos(WM->getStrategicPosition(),PS->getPlayerWhenToTurnAngle());//VAI PARA POSIÇÂO ESTRATEGICA
                                ACT->putCommandInQueue(soc);
                                ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL,soc));
                                
                        }
                }
                        
               }else if(WM->isDeadBallThem()){ //BOLA PARADA DELES, SÒ  CONFIRMAR
                if(WM->isFreeKickThem()){
                }
                if(WM->isCornerKickThem()){
                }
                if(WM->isOffsideThem()){
                }
                if(WM->isKickInThem()){
                }
                if(WM->isFreeKickFaultThem()){
                }
                if(WM->isFoulChargeThem()){
                }
                if(WM->isKickOffThem()){
                }
                if(WM->isBackPassThem()){
                }
                if(WM->isGoalKickThem()){
                }
                if(WM->isPenaltyThem()){
                }
            }
        } else if(WM->isBallKickable()){ // BOLA NÃO TA PARADA E É CHUTÁVEL
                    if(getRegion(agente)==1){ //O JOGADOR ESTÁ NA POSIÇÃO 1
                        VecPosition melhortoque = calculatoque(); /*Funcao: retorna a posição do melhor companheiro*/
                        if(temLadrao()==false){  //Verifica se existe algum inimigo perto
                                 soc = dribble ( 0 , DRIBBLE_WITHBALL);
                                 ACT->putCommandInQueue(soc);
                        } else if(melhortoque!=0  && melhortoque.getX()>posAgent.getX()){
                                 soc = directPass( melhortoque, PASS_FAST);   //Toca rápido para o melhor companheiro (rápido)
                                 ACT->putCommandInQueue(soc);
                                 ACT->putCommandInQueue(turnNeckToPoint(melhortoque,soc));
                                
                                }else{ 
                                 soc = directPass(melhortoque,PASS_FAST);
                                 ACT->putCommandInQueue(soc);
                                 ACT->putCommandInQueue(turnNeckToPoint(melhortoque,soc)); 
                                }
                     } else if(getRegion(agente)==2){ //POSIÇÃO 2
                                 VecPosition posGoalx = posGol();
                                if(direcaolivre(posGoalx)){
                                 soc = kickTo(posGoalx,2);
                                 ACT->putCommandInQueue(soc);
                                 ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL,soc));
                               
                                 }
                                 else{
                                 VecPosition melhortoquepos2 = calculatoque2();
                                    if(melhortoquepos2!=0 && melhortoquepos2.getX()>posAgent.getX()){
                                 soc = directPass( melhortoquepos2, PASS_NORMAL);   //Toca rápido para o melhor companheiro (rápido)
                                 ACT->putCommandInQueue(soc);
                                 ACT->putCommandInQueue(turnNeckToPoint(melhortoquepos2,soc));
                                    }else{
                                 soc = dribble ( /*angulo*/ 0 , DRIBBLE_WITHBALL);
                                 ACT->putCommandInQueue(soc);
                                 
                                     }

                                 }
                             } else if(getRegion(agente)==3){ // POSIÇÃO 3
                                 VecPosition melhortoquepos3 = calculatoque3();
                                     if(melhortoquepos3!=0 && melhortoquepos3.getX()>posAgent.getX()){
                                 soc = directPass( melhortoquepos3, PASS_NORMAL);   //Toca rápido para o melhor companheiro (rápido)
                                 ACT->putCommandInQueue(soc);
                                 ACT->putCommandInQueue(turnNeckToPoint(melhortoquepos3,soc));       
                                     } else{
                                 vaiLinhadeFundo(); // Vai para linha de fundo
                                       }
                                    }
                                else if(getRegion(agente)==4){ // POSIÇÂO 4
                                 VecPosition melhortoquepos4 = calculatoque4();
                                     if(melhortoquepos4!=0 && melhortoquepos4.getX()>posAgent.getX()){
                                 soc = directPass( melhortoquepos4, PASS_NORMAL);   //Toca rápido para o melhor companheiro (rápido)
                                 ACT->putCommandInQueue(soc);
                                 ACT->putCommandInQueue(turnNeckToPoint(melhortoquepos4,soc));  
                                     } else{
                                 vaiLinhadeFundo();
                                       } 

                                     }
                               


                                } else { //BOLA NÂO É CHUTÀVEL
                                     if( WM->getFastestInSetTo( OBJECT_SET_TEAMMATES, OBJECT_BALL, &iTmp )== WM->getAgentObjectType() || WM->getClosestInSetTo(OBJECT_SET_TEAMMATES,OBJECT_BALL,&dist,-1.0)==WM->getAgentObjectType())        {            // sou o mais rápido , posso pegar a bola.
                                 soc = intercept(false);
                                 ACT->putCommandInQueue(soc);
                                 ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL,soc));
                                     }
                       // else if(temLadrao() == false){ // 
                         //      soc = intercept(false);
                           //    ACT->putCommandInQueue(soc);
                            //   ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL,soc));}
                                     else {
                                          //if(verificaLonge()){
                                 soc = moveToPos(WM->getStrategicPosition(),PS->getPlayerWhenToTurnAngle());//VAI PARA POSIÇÂO ESTRATEGICA
                                 ACT->putCommandInQueue(soc);
                                 ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL,soc));
                                  //        } else{
                                    //      if(alguemjuntocomadv()){
                                /*    VecPosition posadvdesmarcado =  procuraradvdesmarcado();
                                    ObjectT advdesmarcado = WM->getClosestInSetTo(OBJECT_SET_TEAMMATES,posadvdesmarcado,&dist,-1.0);
                                    if(WM->getPlayerNumber()==2){
                                    ObjectT marcar = WM->getClosestInSetTo(OBJECT_SET_OPPONENTS,OBJECT_TEAMMATE_2,&dist,-1.0);
                                    soc = mark(marcar,dist,MARK_BISECTOR);
                                    ACT->putCommandInQueue(soc);
                                    ACT->putCommandInQueue(turnNeckToObject(marcar,soc));
                                    }
                                    else if(WM->getPlayerNumber()==3){
                                    ObjectT marcar = WM->getClosestInSetTo(OBJECT_SET_OPPONENTS,OBJECT_TEAMMATE_3,&dist,-1.0);
                                    soc = mark(OBJECT_TEAMMATE_10,dist,MARK_BALL);
                                    ACT->putCommandInQueue(soc);
                                    ACT->putCommandInQueue(turnNeckToObject(marcar,soc));
                                    }
                                     else if(WM->getPlayerNumber()==4){
                                    ObjectT marcar = WM->getClosestInSetTo(OBJECT_SET_OPPONENTS,OBJECT_TEAMMATE_4,&dist,-1.0);
                                    soc = mark(OBJECT_TEAMMATE_9,dist,MARK_BALL);
                                    ACT->putCommandInQueue(soc);
                                    ACT->putCommandInQueue(turnNeckToObject(marcar,soc));
                                    }
                                     else if(WM->getPlayerNumber()==5){
                                    ObjectT marcar = WM->getClosestInSetTo(OBJECT_SET_OPPONENTS,OBJECT_TEAMMATE_5,&dist,-1.0);
                                    soc = mark(OBJECT_TEAMMATE_8,dist,MARK_BALL);
                                    ACT->putCommandInQueue(soc);
                                    ACT->putCommandInQueue(turnNeckToObject(marcar,soc));
                                    }
                                     else if(WM->getPlayerNumber()==6){
                                    ObjectT marcar = WM->getClosestInSetTo(OBJECT_SET_OPPONENTS,OBJECT_TEAMMATE_6,&dist,-1.0);
                                    soc = mark(OBJECT_TEAMMATE_7,dist,MARK_BALL);
                                    ACT->putCommandInQueue(soc);
                                    ACT->putCommandInQueue(turnNeckToObject(marcar,soc));
                                    }
                                     else if(WM->getPlayerNumber()==7){
                                    ObjectT marcar = WM->getClosestInSetTo(OBJECT_SET_OPPONENTS,OBJECT_TEAMMATE_7,&dist,-1.0);
                                    soc = mark(OBJECT_TEAMMATE_5,dist,MARK_BALL);
                                    ACT->putCommandInQueue(soc);
                                    ACT->putCommandInQueue(turnNeckToObject(marcar,soc));
                                    }
                                     else if(WM->getPlayerNumber()==8){
                                    ObjectT marcar = WM->getClosestInSetTo(OBJECT_SET_OPPONENTS,OBJECT_TEAMMATE_8,&dist,-1.0);
                                    soc = mark(OBJECT_TEAMMATE_4,dist,MARK_BALL);
                                    ACT->putCommandInQueue(soc);
                                    ACT->putCommandInQueue(turnNeckToObject(marcar,soc));
                                    }
                                     else if(WM->getPlayerNumber()==9){
                                    ObjectT marcar = WM->getClosestInSetTo(OBJECT_SET_OPPONENTS,OBJECT_TEAMMATE_9,&dist,-1.0);
                                    soc = mark(OBJECT_TEAMMATE_3,dist,MARK_BALL);
                                    ACT->putCommandInQueue(soc);
                                    ACT->putCommandInQueue(turnNeckToObject(marcar,soc));
                                    }
                                     else if(WM->getPlayerNumber()==10){
                                    ObjectT marcar = WM->getClosestInSetTo(OBJECT_SET_OPPONENTS,OBJECT_TEAMMATE_10,&dist,-1.0);
                                    soc = mark(OBJECT_TEAMMATE_2,dist,MARK_BISECTOR);
                                    ACT->putCommandInQueue(soc);
                                    ACT->putCommandInQueue(turnNeckToObject(OBJECT_TEAMMATE_2,soc));
                                    }
                                     else if(WM->getPlayerNumber()==11){
                                    ObjectT marcar = WM->getClosestInSetTo(OBJECT_SET_OPPONENTS,OBJECT_TEAMMATE_11,&dist,-1.0);
                                    soc = mark(marcar,dist,MARK_BALL);
                                    ACT->putCommandInQueue(soc);
                                    ACT->putCommandInQueue(turnNeckToObject(marcar,soc));
                                    } */
                                    
                                  // if(alguemjuntocomadv()){
                                   //   soc = moveToPos(WM->getStrategicPosition(),20); // move para posição estratégica
                                   // }else{
                                     //   soc = mark(advdesmarcado,dist,MARK_BALL); // marca o
                                       // ACT->putCommandInQueue(soc);
                                      //  ACT->putCommandInQueue(turnNeckToObject(advdesmarcado,soc));
                                        
                                    }
                                //}else{
                                 //  soc = moveToPos(posBall,PS->getPlayerWhenToTurnAngle());
                                 //  ACT->putCommandInQueue(soc);
                                  // ACT->putCommandInQueue(turnNeckToObject(OBJECT_BALL,soc));
                                //}
                            }
                        }

               // }

        //}
        
 //}
 
 return soc;
 
 }
/*
else if( WM->getFastestInSetTo( OBJECT_SET_TEAMMATES, OBJECT_BALL, &iTmp )
              == WM->getAgentObjectType()  && !WM->isDeadBallThem() )
    {                                                // if fastest to ball
      Log.log( 100, "I am fastest to ball; can get there in %d cycles", iTmp );
      soc = intercept( false );                      // intercept the ball

      if( soc.commandType == CMD_DASH &&             // if stamina low
          WM->getAgentStamina().getStamina() <
             SS->getRecoverDecThr()*SS->getStaminaMax()+200 )
      {
        soc.dPower = 30.0 * WM->getAgentStamina().getRecovery(); // dash slow
        ACT->putCommandInQueue( soc );
        ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
      }
      else                                           // if stamina high
      {
        ACT->putCommandInQueue( soc );               // dash as intended
        ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
      }
     }
     else if( posAgent.getDistanceTo(WM->getStrategicPosition()) >
                  1.5 + fabs(posAgent.getX()-posBall.getX())/10.0)
                                                  // if not near strategic pos
     {
       if( WM->getAgentStamina().getStamina() >     // if stamina high
                            SS->getRecoverDecThr()*SS->getStaminaMax()+800 )
       {
         soc = moveToPos(WM->getStrategicPosition(),
                         PS->getPlayerWhenToTurnAngle());
         ACT->putCommandInQueue( soc );            // move to strategic pos
         ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
       }
       else                                        // else watch ball
       {
         ACT->putCommandInQueue( soc = turnBodyToObject( OBJECT_BALL ) );
         ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
       }
     }
     else if( fabs( WM->getRelativeAngle( OBJECT_BALL ) ) > 1.0 ) // watch ball
     {
       ACT->putCommandInQueue( soc = turnBodyToObject( OBJECT_BALL ) );
       ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
     }
     else                                         // nothing to do
       ACT->putCommandInQueue( SoccerCommand(CMD_TURNNECK,0.0) );
   }
   
  return soc;
}



*/



/*!This method is a simple goalie based on the goalie of the simple Team of
   FC Portugal. It defines a rectangle in its penalty area and moves to the
   position on this rectangle where the ball intersects if you make a line
   between the ball position and the center of the goal. If the ball can
   be intercepted in the own penalty area the ball is intercepted and catched.
*/
SoccerCommand Player::deMeer5_goalie(  )
{
  int i;

  SoccerCommand soc;
  VecPosition   posAgent = WM->getAgentGlobalPosition();
  AngDeg        angBody  = WM->getAgentGlobalBodyAngle();

  // define the top and bottom position of a rectangle in which keeper moves
  static const VecPosition posLeftTop( -PITCH_LENGTH/2.0 +
               0.7*PENALTY_AREA_LENGTH, -PENALTY_AREA_WIDTH/4.0 );
  static const VecPosition posRightTop( -PITCH_LENGTH/2.0 +
               0.7*PENALTY_AREA_LENGTH, +PENALTY_AREA_WIDTH/4.0 );

  // define the borders of this rectangle using the two points.
  static Line  lineFront = Line::makeLineFromTwoPoints(posLeftTop,posRightTop);
  static Line  lineLeft  = Line::makeLineFromTwoPoints(
                         VecPosition( -50.0, posLeftTop.getY()), posLeftTop );
  static Line  lineRight = Line::makeLineFromTwoPoints(
                         VecPosition( -50.0, posRightTop.getY()),posRightTop );


  if( WM->isBeforeKickOff( ) )
  {
    if( formations->getFormation() != FT_INITIAL || // not in kickoff formation
        posAgent.getDistanceTo( WM->getStrategicPosition() ) > 2.0 )  
    {
      formations->setFormation( FT_INITIAL );       // go to kick_off formation
      ACT->putCommandInQueue( soc=teleportToPos(WM->getStrategicPosition()) );
    }
    else                                            // else turn to center
    {
      ACT->putCommandInQueue( soc = turnBodyToPoint( VecPosition( 0, 0 ), 0 ));
      ACT->putCommandInQueue( alignNeckWithBody( ) );
    }
    return soc;
  }

  if( WM->getConfidence( OBJECT_BALL ) < PS->getBallConfThr() )
  {                                                // confidence ball too  low
    ACT->putCommandInQueue( searchBall() );        // search ball
    ACT->putCommandInQueue( alignNeckWithBody( ) );
  }
  else if( WM->getPlayMode() == PM_PLAY_ON || WM->isFreeKickThem() ||
           WM->isCornerKickThem() )               
  {
    if( WM->isBallCatchable() )
    {
      ACT->putCommandInQueue( soc = catchBall() );
      ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
    }
     else if( WM->isBallKickable() )
    {
       soc = kickTo( VecPosition(0,posAgent.getY()*2.0), 2.0 );    
       ACT->putCommandInQueue( soc );
       ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
    }
    else if( WM->isInOwnPenaltyArea( getInterceptionPointBall( &i, true ) ) &&
             WM->getFastestInSetTo( OBJECT_SET_PLAYERS, OBJECT_BALL, &i ) == 
                                               WM->getAgentObjectType() )
    {
      ACT->putCommandInQueue( soc = intercept( true ) );
      ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
    }
    else
    {
      // make line between own goal and the ball
      VecPosition posMyGoal = ( WM->getSide() == SIDE_LEFT )
             ? SoccerTypes::getGlobalPositionFlag(OBJECT_GOAL_L, SIDE_LEFT )
             : SoccerTypes::getGlobalPositionFlag(OBJECT_GOAL_R, SIDE_RIGHT);
      Line lineBall = Line::makeLineFromTwoPoints( WM->getBallPos(),posMyGoal);

      // determine where your front line intersects with the line from ball
      VecPosition posIntersect = lineFront.getIntersection( lineBall );

      // outside rectangle, use line at side to get intersection
      if (posIntersect.isRightOf( posRightTop ) )
        posIntersect = lineRight.getIntersection( lineBall );
      else if (posIntersect.isLeftOf( posLeftTop )  )
        posIntersect = lineLeft.getIntersection( lineBall );

      if( posIntersect.getX() < -49.0 )
        posIntersect.setX( -49.0 );
        
      // and move to this position
      if( posIntersect.getDistanceTo( WM->getAgentGlobalPosition() ) > 0.5 )
      {
        soc = moveToPos( posIntersect, PS->getPlayerWhenToTurnAngle() );
        ACT->putCommandInQueue( soc );
        ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
      }
      else
      {
        ACT->putCommandInQueue( soc = turnBodyToObject( OBJECT_BALL ) );
        ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
      }
    }
  }
  else if( WM->isFreeKickUs() == true || WM->isGoalKickUs() == true )
  {
    if( WM->isBallKickable() )
    {
      if( WM->getTimeSinceLastCatch() == 25 && WM->isFreeKickUs() )
      {
        // move to position with lesser opponents.
        if( WM->getNrInSetInCircle( OBJECT_SET_OPPONENTS, 
                                          Circle(posRightTop, 15.0 )) <
            WM->getNrInSetInCircle( OBJECT_SET_OPPONENTS, 
                                           Circle(posLeftTop,  15.0 )) )
          soc.makeCommand( CMD_MOVE,posRightTop.getX(),posRightTop.getY(),0.0);
        else
          soc.makeCommand( CMD_MOVE,posLeftTop.getX(), posLeftTop.getY(), 0.0);
        ACT->putCommandInQueue( soc );
      }
      else if( WM->getTimeSinceLastCatch() > 28 )
      {
        soc = kickTo( VecPosition(0,posAgent.getY()*2.0), 2.0 );    
        ACT->putCommandInQueue( soc );
      }
      else if( WM->getTimeSinceLastCatch() < 25 )
      {
        VecPosition posSide( 0.0, posAgent.getY() ); 
        if( fabs( (posSide - posAgent).getDirection() - angBody) > 10 )
        {
          soc = turnBodyToPoint( posSide );
          ACT->putCommandInQueue( soc );
        }
        ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
      }
    }
    else if( WM->isGoalKickUs()  )
    {
      ACT->putCommandInQueue( soc = intercept( true ) );
      ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
    }
    else
      ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
  }
  else
  {
     ACT->putCommandInQueue( soc = turnBodyToObject( OBJECT_BALL ) );
     ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
  }
  return soc;
}
