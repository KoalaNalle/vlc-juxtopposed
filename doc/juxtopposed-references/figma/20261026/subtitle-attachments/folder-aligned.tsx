const assetPathPrefix = "https://www.figma.com/api/mcp/asset/a99c5734-520a-4285-bb33-60e76415db2a";
const imgFrame1 = `${assetPathPrefix}/1b0f7.svg`;
const imgFrame = `${assetPathPrefix}/cf6e1.svg`;
const imgFrame2 = `${assetPathPrefix}/764c9.svg`;
const imgFrame3 = `${assetPathPrefix}/d74b3.svg`;
const imgFrame4 = `${assetPathPrefix}/9c2da.svg`;
const imgFrame5 = `${assetPathPrefix}/a8689.svg`;
const imgVector77 = `${assetPathPrefix}/b0920.svg`;
const imgVector78 = `${assetPathPrefix}/a421b.svg`;
const imgVector79 = `${assetPathPrefix}/05902.svg`;
const imgVector80 = `${assetPathPrefix}/03b9d.svg`;
const imgVector81 = `${assetPathPrefix}/c3882.svg`;
const imgFrame6 = `${assetPathPrefix}/8abe2.svg`;

type IconProps = {
  className?: string;
  direction?: "default" | "down";
  eq?: "default";
  name?: "settings" | "chevron" | "check" | "folder";
  state?: "default" | "active";
  twoTone?: "no";
  value?: "default";
};

function Icon({ className, direction = "default", eq = "default", name = "settings", state = "default", twoTone = "no", value = "default" }: IconProps) {
  const isCheckAndDefaultAndActiveAndDefaultAndNo = name === "check" && direction === "default" && state === "active" && eq === "default" && twoTone === "no";
  const isCheckAndDefaultAndDefaultAndDefaultAndNo = name === "check" && direction === "default" && state === "default" && eq === "default" && twoTone === "no";
  const isChevronAndDownAndDefaultAndDefaultAndNo = name === "chevron" && direction === "down" && state === "default" && eq === "default" && twoTone === "no";
  const isDefaultAndDefaultAndNoAndIsFolderAndDefaultOrChevronAndDown = state === "default" && eq === "default" && twoTone === "no" && ((name === "folder" && direction === "default") || (name === "chevron" && direction === "down"));
  const isFolderAndDefaultAndDefaultAndDefaultAndNo = name === "folder" && direction === "default" && state === "default" && eq === "default" && twoTone === "no";
  return (
    <div className={className || "content-stretch flex items-center justify-center relative"} id={isCheckAndDefaultAndDefaultAndDefaultAndNo ? "node-494_2400" : isCheckAndDefaultAndActiveAndDefaultAndNo ? "node-494_2394" : isChevronAndDownAndDefaultAndDefaultAndNo ? "node-71_868" : isFolderAndDefaultAndDefaultAndDefaultAndNo ? "node-51_808" : "node-42_355"}>
      <div className={`relative shrink-0 ${isDefaultAndDefaultAndNoAndIsFolderAndDefaultOrChevronAndDown ? "flex items-center justify-center" : "size-[24px]"}`} id={isCheckAndDefaultAndDefaultAndDefaultAndNo ? "node-494_2401" : isCheckAndDefaultAndActiveAndDefaultAndNo ? "node-494_2395" : isChevronAndDownAndDefaultAndDefaultAndNo ? "node-71_869" : isFolderAndDefaultAndDefaultAndDefaultAndNo ? "node-51_809" : "node-44_413"}>
        {direction === "default" && eq === "default" && twoTone === "no" && ((name === "settings" && state === "default") || (name === "check" && state === "active")) && <img alt="" className="absolute block inset-0 max-w-none size-full" src={name === "check" && state === "active" && eq === "default" && twoTone === "no" ? imgFrame3 : imgFrame1} />}
        {isDefaultAndDefaultAndNoAndIsFolderAndDefaultOrChevronAndDown && (
          <div className="-scale-y-100 flex-none rotate-180">
            <div className="relative size-[24px]" data-name="Frame">
              <img alt="" className="absolute block inset-0 max-w-none size-full" src={name === "chevron" && direction === "down" && eq === "default" && twoTone === "no" ? imgFrame2 : imgFrame} />
            </div>
          </div>
        )}
      </div>
    </div>
  );
}

type TargetProps = {
  className?: string;
  state?: "to this folder";
};

function Target({ className, state = "to this folder" }: TargetProps) {
  return (
    <button className={className || "bg-[#151515] content-stretch cursor-pointer flex items-center justify-between pl-[12px] pr-[8px] py-[5px] relative rounded-[6px] w-[200px]"} data-node-id="1959:5324">
      <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] text-left whitespace-nowrap" data-node-id="1959:5320">
        To This Folder
      </p>
      <Icon className="content-stretch flex items-center justify-center relative shrink-0" direction="down" name="chevron" />
    </button>
  );
}

export default function AddSubtitlesToPlaylistFolder() {
  return (
    <div className="backdrop-blur-[25px] bg-[rgba(14,14,14,0.95)] content-stretch flex flex-col isolate items-start relative rounded-[14px] shadow-[0px_8px_15px_0px_rgba(0,0,0,0.46)] size-full" data-node-id="1911:7545" data-name="Add Subtitles to Playlist/Folder">
      <div className="bg-[#090909] content-stretch flex flex-col gap-[12px] items-start p-[9px] relative rounded-tl-[14px] rounded-tr-[14px] shrink-0 w-full z-[3]" data-node-id="1911:7546">
        <div className="content-stretch flex items-center justify-between pl-[10px] relative shrink-0 w-full" data-node-id="1911:7767">
          <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#8f8f8f] text-[14px] whitespace-nowrap" data-node-id="1911:7766">
            Add Subtitles
          </p>
          <button className="content-stretch cursor-pointer flex items-center justify-center p-[4px] relative rounded-[4px] shrink-0 w-[32px]" data-node-id="1911:7560" data-name="Icon Wrapper">
            <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1911:7560;177:88" data-name="Icon">
              <div className="flex items-center justify-center relative shrink-0" data-node-id="I1911:7560;177:88;45:41">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame4} />
                  </div>
                </div>
              </div>
            </div>
          </button>
        </div>
        <div className="content-stretch flex gap-[10px] items-center relative shrink-0 w-full" data-node-id="1911:7765">
          <Target className="bg-[#151515] content-stretch cursor-pointer flex items-center justify-between pl-[12px] pr-[8px] py-[5px] relative rounded-[6px] shrink-0 w-[200px]" />
          <div className="bg-[#151515] content-stretch flex items-center justify-between pl-[12px] pr-[8px] py-[5px] relative rounded-[6px] shrink-0 w-[200px]" data-node-id="1919:9359">
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1919:9360">
              From Computer
            </p>
            <Icon className="content-stretch flex items-center justify-center relative shrink-0" direction="down" name="chevron" />
          </div>
          <div className="bg-[#151515] content-stretch flex flex-[1_0_0] gap-[10px] h-[34px] items-center min-w-px px-[8px] py-[5px] relative rounded-[6px]" data-node-id="1911:7755">
            <Icon className="content-stretch flex items-center justify-center relative shrink-0" name="folder" />
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1911:7757">
              Videos / Shows / The Wand / S01
            </p>
          </div>
        </div>
      </div>
      <div className="content-stretch flex flex-[1_0_0] items-center min-h-px relative w-full z-[2]" data-node-id="1911:7561">
        <div className="content-stretch flex flex-[1_0_0] flex-col gap-[16px] h-full items-start min-w-px overflow-clip pl-[30px] py-[30px] relative" data-node-id="1911:7562">
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="1911:7853">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="1911:7854" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I1911:7854;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1911:7855">
              The_Wand_S01E01.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="1911:7954">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector77} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="1911:7782">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="1911:7772" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I1911:7772;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1911:7781">
              The_Wand_S01E02.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="1911:7859">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector78} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="1911:7783">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="1911:7784" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I1911:7784;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1911:7785">
              The_Wand_S01E03.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="1911:7944">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector78} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="1911:7788">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="1911:7789" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I1911:7789;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1911:7790">
              The_Wand_S01E04.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="1911:7947">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector78} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="1911:7793">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="1911:7794" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I1911:7794;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1911:7795">
              The_Wand_S01E05.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="1911:7953">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector79} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="1911:7798">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="1911:7799" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I1911:7799;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1911:7800">
              The_Wand_S01E06.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="1911:7948">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector79} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="1911:7803">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="1911:7804" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I1911:7804;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1911:7805">
              The_Wand_S01E07.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="1911:7945">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector77} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="1911:7808">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="1911:7809" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I1911:7809;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1911:7810">
              The_Wand_S01E08.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="1911:7951">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector78} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="1911:7813">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="1911:7814" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I1911:7814;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1911:7815">
              The_Wand_S01E09.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="1911:7952">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector78} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="1911:7818">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="1911:7819" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I1911:7819;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1911:7820">
              The_Wand_S01E10.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="1911:7949">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector77} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="1911:7823">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="1911:7824" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I1911:7824;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1911:7825">
              The_Wand_S01E11.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="1911:7950">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector80} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="1911:7828">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="1911:7829" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I1911:7829;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1911:7830">
              The_Wand_S01E12.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="1911:7946">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector77} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="1911:7833">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="1911:7834" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I1911:7834;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1911:7835">
              The_Wand_S01E13.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="1911:7956">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector81} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="1911:7838">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="1911:7839" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I1911:7839;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1911:7840">
              The_Wand_S01E14.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="1911:7957">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector77} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="1911:7843">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="1911:7844" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I1911:7844;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="1911:7845">
              The_Wand_S01E15.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="1911:7958">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector77} />
              </div>
            </div>
          </div>
        </div>
        <div className="content-stretch flex flex-[1_0_0] flex-col gap-[4px] h-full items-start min-w-px overflow-clip px-[20px] py-[22px] relative" data-node-id="1911:7861">
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="1915:8446" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I1915:8446;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1915:8446;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I1915:8446;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I1915:8446;1915:8386">
                The_Wand_S01E01.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="1915:8459" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I1915:8459;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1915:8459;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I1915:8459;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I1915:8459;1915:8386">
                The_Wand_S01E02.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="1915:8472" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I1915:8472;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1915:8472;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I1915:8472;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I1915:8472;1915:8386">
                The_Wand_S01E03.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="1915:8485" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I1915:8485;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1915:8485;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I1915:8485;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I1915:8485;1915:8386">
                The_Wand_S01E04.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="1915:8498" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I1915:8498;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1915:8498;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I1915:8498;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I1915:8498;1915:8386">
                The_Wand_S01E05.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="1915:8511" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I1915:8511;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1915:8511;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I1915:8511;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I1915:8511;1915:8386">
                The_Wand_S01E06.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="1915:8524" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I1915:8524;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1915:8524;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I1915:8524;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I1915:8524;1915:8386">
                The_Wand_S01E07.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="1915:8537" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I1915:8537;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1915:8537;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I1915:8537;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I1915:8537;1915:8386">
                The_Wand_S01E08.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="1915:8550" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I1915:8550;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1915:8550;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I1915:8550;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I1915:8550;1915:8386">
                The_Wand_S01E09.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="1915:8563" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I1915:8563;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1915:8563;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I1915:8563;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I1915:8563;1915:8386">
                The_Wand_S01E10.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="1915:8576" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I1915:8576;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1915:8576;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I1915:8576;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I1915:8576;1915:8386">
                The_Wand_S01E11.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="1915:8589" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I1915:8589;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1915:8589;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I1915:8589;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I1915:8589;1915:8386">
                The_Wand_S01E12.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="1915:8602" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I1915:8602;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1915:8602;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I1915:8602;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I1915:8602;1915:8386">
                The_Wand_S01E13.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="1915:8615" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I1915:8615;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1915:8615;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I1915:8615;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I1915:8615;1915:8386">
                The_Wand_S01E14.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="1915:8628" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I1915:8628;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I1915:8628;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I1915:8628;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I1915:8628;1915:8386">
                The_Wand_S01E15.srt
              </p>
            </div>
          </div>
        </div>
      </div>
      <div className="content-stretch flex items-center justify-between p-[20px] relative shrink-0 w-full z-[1]" data-node-id="1915:8662">
        <div className="bg-[#1b1b1b] content-stretch flex h-[32px] items-center justify-center px-[8px] py-[4px] relative rounded-[6px] shrink-0 w-[100px]" data-node-id="1915:8659">
          <p className="[word-break:break-word] font-['Inter:Semi_Bold'] font-semibold leading-[normal] not-italic relative shrink-0 text-[#8f8f8f] text-[14px] whitespace-nowrap" data-node-id="1915:8660">
            Cancel
          </p>
        </div>
        <button className="bg-[#371f06] content-stretch cursor-pointer flex h-[32px] items-center justify-center px-[8px] py-[4px] relative rounded-[6px] shrink-0 w-[100px]" data-node-id="1915:8663">
          <p className="[word-break:break-word] font-['Inter:Semi_Bold'] font-semibold leading-[normal] not-italic relative shrink-0 text-[#d37918] text-[14px] text-left whitespace-nowrap" data-node-id="1915:8664">
            Save
          </p>
        </button>
      </div>
    </div>
  );
}